#include <unistd.h>
#include <csignal>
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include <sys/stat.h> // For mkdir

// ROOT Headers
#include "TObjString.h"
#include "TObjArray.h"
#include "TStopwatch.h"
#include "TMath.h"
#include "TUnixSystem.h"
#include "TMessage.h"
#include "TSystem.h" 

// Project Headers
#include "frontend.hh"
#include "FadcBD.hh"
#include "RawChannel.hh"

using namespace std;

// -----------------------------------------------------------------------------
// Global Variable Definitions
// -----------------------------------------------------------------------------
NK6UVME    _kvme;
NKNFADC400 _kadc;
int        _nkusb = 0;

RunInfo * _runinfo = NULL;
ELog       _log;

TFile * _outputfile = NULL;
TTree * _rawdatatree = NULL;
TClonesArray * _rawdata = NULL;

unsigned long long _ttime = 0;
unsigned int       _tpattern = 0;
int                _event_id = 0;

TSocket * _socket = NULL;
bool      _havedisplay = false;

TObjArray * _outputfilelist = NULL;
const char * _outputfilename = NULL;
int          _noutputfile = 0;
bool         _doneconfiguration = false;

unsigned long _neventperdump = 0;
unsigned long _recordlength = 0;

THashTable * _triggerlookup = NULL;
bool _is_running = false;

// -----------------------------------------------------------------------------
// Signal Handler
// -----------------------------------------------------------------------------
void signal_handler(int signum) {
    if (_is_running) {
        cout << "\n>>> Stop signal received (Ctrl+C). Finalizing run..." << endl;
        _is_running = false;
    }
}

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------
bool TestBit(unsigned int n, unsigned int i) {
    return ((n) & (1 << i)) != 0;
}

void PrintUsage() {
    cout << "Usage: frontend_nfadc400 [-f config] [-o output] [-n events | -t seconds]" << endl;
    cout << "  Defaults if no args:" << endl;
    cout << "    Config : configs/daq.config" << endl;
    cout << "    Output : data/test_run.root" << endl;
    cout << "    Time   : 10 seconds" << endl;
}

void ShowRegisters(FadcBD * bd) {
    unsigned long mid = bd->MID();
    int nch = bd->NCHANNEL();

    int rl  = _kadc.NFADC400read_RL(_nkusb, mid);
    int tlt = _kadc.NFADC400read_TLT(_nkusb, mid);
    int tow = _kadc.NFADC400read_TOW(_nkusb, mid);
    int rst = _kadc.NFADC400read_RM(_nkusb, mid);

    cout << " -------------------------------------------------------" << endl;
    cout << Form("     Current Registers of FADC [MID=%lu]", mid) << endl;
    cout << " -------------------------------------------------------" << endl;
    cout << Form(" %8s %8s %8s %8s %8s", "NCH","RL","RST","TLT","TOW") << endl; 
    cout << Form(" %8d %8d %8d %8d %8d",  nch, rl, rst, tlt, tow) << endl;
    cout << " -------------------------------------------------------" << endl;
    
    cout << "   THR: ";
    for(int i=0; i<nch; i++) cout << setw(6) << _kadc.NFADC400read_THR(_nkusb, mid, bd->CID(i)+1);
    cout << endl << "   DLY: ";
    for(int i=0; i<nch; i++) cout << setw(6) << _kadc.NFADC400read_DLY(_nkusb, mid, bd->CID(i)+1);
    cout << endl << "   POL: ";
    for(int i=0; i<nch; i++) cout << setw(6) << _kadc.NFADC400read_POL(_nkusb, mid, bd->CID(i)+1);
    cout << endl;
    
    _kadc.NFADC400measure_PED(_nkusb, mid, 0); 
    cout << "   PED: ";
    for(int i=0; i<nch; i++) cout << setw(6) << _kadc.NFADC400read_PED(_nkusb, mid, bd->CID(i)+1);
    cout << endl;
    cout << " -------------------------------------------------------" << endl;
}

void PrepareOutputEnvironment(const string& configFile, const string& outputFile) {
    TString outPath(outputFile);
    TString dirName = gSystem->DirName(outPath);
    
    if (gSystem->AccessPathName(dirName)) { 
        _log.Print(ELog::INFO, Form("Creating directory: %s", dirName.Data()));
        gSystem->mkdir(dirName, true); 
    }

    TString backupPath = outPath;
    if (backupPath.EndsWith(".root")) backupPath.ReplaceAll(".root", ".config");
    else backupPath += ".config";

    ifstream src(configFile, ios::binary);
    ofstream dst(backupPath.Data(), ios::binary);

    if (src && dst) {
        dst << src.rdbuf();
        _log.Print(ELog::INFO, Form("Backup Config Saved: %s", backupPath.Data()));
    } else {
        _log.Print(ELog::WARNING, "Failed to backup config file!");
    }
    src.close();
    dst.close();
}

// -----------------------------------------------------------------------------
// Initialization & Config
// -----------------------------------------------------------------------------
void Initialize() {
    _havedisplay = true;
    _doneconfiguration = false;
    _outputfilename = NULL;
    _outputfile = NULL;
    _noutputfile = 0;
    
    _outputfilelist = new TObjArray();
    _runinfo = new RunInfo();
    _triggerlookup = new THashTable();
    _triggerlookup->SetOwner(kTRUE);
    
    for(unsigned int i=0; i<4; i++) {
        unsigned int bit = 0;
        for(unsigned int n=0; n<16; n++) if(TestBit(n,i)) bit |= (1<<n);
        _triggerlookup->Add(new Node(Form("%d", i), bit));
    }
    _triggerlookup->Add(new Node("OR", 0xFFFE));
    _triggerlookup->Add(new Node("AND", 0x8000));

    _log.Print(ELog::INFO, "System Initialized.");
}

void Configuration_byTicket(const char * filename) {
    ifstream ticket(filename);
    if (!ticket.is_open()) {
        _log.Print(ELog::ERROR, Form("Cannot open config file: %s", filename));
        exit(1);
    }
    _log.Print(ELog::INFO, Form("Loading Configuration: %s", filename));
    
    string line;
    while(getline(ticket, line)) {
        if(line.find("#") != string::npos) line = line.substr(0, line.find("#"));
        if(line.empty()) continue;
        TString ts(line);
        if(ts.IsWhitespace()) continue;
        TObjArray * token = ts.Tokenize(" \t");
        if(token->GetEntries() == 0) { delete token; continue; }
        
        TString key = ((TObjString*)token->At(0))->String();
        key.ToUpper();
        
        if (key == "RUNNUM") _runinfo->SetRunNumber(((TObjString*)token->At(1))->String().Atoi());
        else if (key == "RUNTYP") _runinfo->SetRunType(((TObjString*)token->At(1))->String().Data());
        else if (key == "RUNDSC") { 
            TString desc = "";
            for(int k=1; k<token->GetEntries(); k++) desc += ((TObjString*)token->At(k))->String() + " ";
            _runinfo->SetRunDesc(desc);
        }
        else if (key == "FADC") {
            int mid = ((TObjString*)token->At(1))->String().Atoi();
            int nch = ((TObjString*)token->At(2))->String().Atoi();
            FadcBD * bd = new FadcBD(mid);
            bd->SetNCH(nch);
            for(int i=0; i<nch; i++) bd->SetCID(i, ((TObjString*)token->At(3+i))->String().Atoi());
            _runinfo->AddFadcBD(bd);
        }
        else if (key == "NDP") {
            int mid = ((TObjString*)token->At(1))->String().Atoi();
            int val = ((TObjString*)token->At(2))->String().Atoi();
            if(_runinfo->GetFadcBD(mid)) _runinfo->GetFadcBD(mid)->SetNDP(val);
        }
        else if (key == "TLT") {
            int mid = ((TObjString*)token->At(1))->String().Atoi();
            TString val = ((TObjString*)token->At(2))->String();
            unsigned int code = 0xFFFE;
            Node* node = (Node*)_triggerlookup->FindObject(val.Data());
            if(node) code = node->GetCode();
            else if(val.BeginsWith("0x")) code = strtoul(val.Data(), NULL, 16);
            else code = val.Atoi();
            if(_runinfo->GetFadcBD(mid)) _runinfo->GetFadcBD(mid)->SetTLT(code);
        }
        else if (key == "THR") {
            int mid = ((TObjString*)token->At(1))->String().Atoi();
            if(_runinfo->GetFadcBD(mid)) for(int i=0; i<_runinfo->GetFadcBD(mid)->NCHANNEL(); i++) if(i+2 < token->GetEntries()) _runinfo->GetFadcBD(mid)->SetTHR(i, ((TObjString*)token->At(2+i))->String().Atoi());
        }
        else if (key == "POL") {
            int mid = ((TObjString*)token->At(1))->String().Atoi();
            if(_runinfo->GetFadcBD(mid)) for(int i=0; i<_runinfo->GetFadcBD(mid)->NCHANNEL(); i++) if(i+2 < token->GetEntries()) _runinfo->GetFadcBD(mid)->SetPOL(i, ((TObjString*)token->At(2+i))->String().Atoi());
        }
        else if (key == "DACOFF") {
            int mid = ((TObjString*)token->At(1))->String().Atoi();
            if(_runinfo->GetFadcBD(mid)) for(int i=0; i<_runinfo->GetFadcBD(mid)->NCHANNEL(); i++) if(i+2 < token->GetEntries()) _runinfo->GetFadcBD(mid)->SetDACOFF(i, ((TObjString*)token->At(2+i))->String().Atoi());
        }
        else if (key == "DLY") {
            int mid = ((TObjString*)token->At(1))->String().Atoi();
            if(_runinfo->GetFadcBD(mid)) for(int i=0; i<_runinfo->GetFadcBD(mid)->NCHANNEL(); i++) if(i+2 < token->GetEntries()) _runinfo->GetFadcBD(mid)->SetDLY(i, ((TObjString*)token->At(2+i))->String().Atoi());
        }
        else if (key == "DT") {
            int mid = ((TObjString*)token->At(1))->String().Atoi();
            if(_runinfo->GetFadcBD(mid)) for(int i=0; i<_runinfo->GetFadcBD(mid)->NCHANNEL(); i++) if(i+2 < token->GetEntries()) _runinfo->GetFadcBD(mid)->SetDT(i, ((TObjString*)token->At(2+i))->String().Atoi());
        }
        else if (key == "CW") {
            int mid = ((TObjString*)token->At(1))->String().Atoi();
            if(_runinfo->GetFadcBD(mid)) for(int i=0; i<_runinfo->GetFadcBD(mid)->NCHANNEL(); i++) if(i+2 < token->GetEntries()) _runinfo->GetFadcBD(mid)->SetCW(i, ((TObjString*)token->At(2+i))->String().Atoi());
        }
        else if (key == "TM") {
            int mid = ((TObjString*)token->At(1))->String().Atoi();
            if(_runinfo->GetFadcBD(mid)) for(int i=0; i<_runinfo->GetFadcBD(mid)->NCHANNEL(); i++) if(i+2 < token->GetEntries()) _runinfo->GetFadcBD(mid)->SetTM(i, ((TObjString*)token->At(2+i))->String().Atoi());
        }
        else if (key == "PCT") {
            int mid = ((TObjString*)token->At(1))->String().Atoi();
            if(_runinfo->GetFadcBD(mid)) for(int i=0; i<_runinfo->GetFadcBD(mid)->NCHANNEL(); i++) if(i+2 < token->GetEntries()) _runinfo->GetFadcBD(mid)->SetPCT(i, ((TObjString*)token->At(2+i))->String().Atoi());
        }
        else if (key == "PCI") {
            int mid = ((TObjString*)token->At(1))->String().Atoi();
            if(_runinfo->GetFadcBD(mid)) for(int i=0; i<_runinfo->GetFadcBD(mid)->NCHANNEL(); i++) if(i+2 < token->GetEntries()) _runinfo->GetFadcBD(mid)->SetPCI(i, ((TObjString*)token->At(2+i))->String().Atoi());
        }
        else if (key == "PWT") {
            int mid = ((TObjString*)token->At(1))->String().Atoi();
            if(_runinfo->GetFadcBD(mid)) for(int i=0; i<_runinfo->GetFadcBD(mid)->NCHANNEL(); i++) if(i+2 < token->GetEntries()) _runinfo->GetFadcBD(mid)->SetPWT(i, ((TObjString*)token->At(2+i))->String().Atoi());
        }

        delete token;
    }
    ticket.close();
    _doneconfiguration = true;
}

bool opennewfile(const char * fname) {
    if(_outputfile) { _outputfile->Write(); _outputfile->Close(); delete _outputfile; }
    TString filename = fname;
    if (!filename.EndsWith(".root")) filename += ".root";

    _outputfile = new TFile(filename, "RECREATE");
    if(!_outputfile->IsOpen()) {
        _log.Print(ELog::ERROR, Form("Cannot create output file: %s", filename.Data()));
        exit(1);
    }
    _log.Print(ELog::INFO, Form("Output File Opened: %s", filename.Data()));
    _runinfo->Write();
    
    _rawdatatree = new TTree("FADC", "FADC Raw Data");
    _rawdatatree->Branch("rawdata", &_rawdata);
    _rawdatatree->Branch("ttime", &_ttime, "ttime/l");
    _rawdatatree->Branch("pattern", &_tpattern, "pattern/i");
    _rawdatatree->Branch("eventid", &_event_id, "eventid/I");
    
    TObjString * fn = new TObjString(filename);
    _outputfilelist->Add(fn);
    _noutputfile++;
    return true;
}

void RunInitialize() {
    _nkusb = 0;
    
    // 1. VME 컨트롤러 연결 (필수)
    if(_kvme.VMEopen(_nkusb) < 0) { 
        _log.Print(ELog::ERROR, "Failed to open USB-VME Controller."); 
        exit(1); 
    }
    
    // 2. 설정된 보드만 순차적으로 초기화 (스캔 로직 제거됨)
    int nbd = _runinfo->GetNFadcBD();
    for(int i=0; i<nbd; i++) {
        FadcBD * fadc = _runinfo->GetFadcBD(i);
        unsigned long mid = fadc->MID();
        int nch = fadc->NCHANNEL();
        
        _log.Print(ELog::INFO, Form("Initializing FADC [MID=%lu]...", mid));
        
        // 보드 오픈
        _kadc.NFADC400open(_nkusb, mid);

        // [최소한의 안전장치] 존재 여부만 조용히 확인
        // 원본처럼 바로 Write를 하면 원인을 모르고 죽으므로, Check만 하나 둡니다.
        unsigned long stat = _kadc.NFADC400read_STAT(_nkusb, mid);
        if (stat != 0xFADC) {
            // 보드가 죽어있으면 여기서 명확히 멈춤
            _log.Print(ELog::ERROR, Form(" >> FADC [MID=%lu] Initialization Failed. (No Response: 0x%lx)", mid, stat));
            _log.Print(ELog::ERROR, " >> Check DIP Switch (MID) or VME Slot Connection.");
            exit(1); 
        }

        // 3. 레지스터 설정 (원본 로직)
        _kadc.NFADC400write_RM(_nkusb, mid, 0, 0, 1);
        _kadc.NFADC400reset(_nkusb, mid);
        
        _kadc.NFADC400write_RL(_nkusb, mid, fadc->RL());
        _kadc.NFADC400write_TLT(_nkusb, mid, fadc->TLT());
        _kadc.NFADC400write_TOW(_nkusb, mid, fadc->TOW());
        
        if(fadc->DCE() == 0) _kadc.NFADC400enable_DCE(_nkusb, mid);
        else                 _kadc.NFADC400disable_DCE(_nkusb, mid);
        
        for(int j=0; j<nch; j++) {
            int cid = fadc->CID(j) + 1;
            _kadc.NFADC400write_DACOFF(_nkusb, mid, cid, fadc->DACOFF(j));
            _kadc.NFADC400write_DACGAIN(_nkusb, mid, cid, fadc->DACGAIN(j));
            _kadc.NFADC400write_DLY(_nkusb, mid, cid, fadc->DLY(j));
            _kadc.NFADC400write_THR(_nkusb, mid, cid, fadc->THR(j));
            _kadc.NFADC400write_POL(_nkusb, mid, cid, fadc->POL(j));
            _kadc.NFADC400write_DT(_nkusb, mid, cid, fadc->DT(j));
            _kadc.NFADC400write_CW(_nkusb, mid, cid, fadc->CW(j));
            _kadc.NFADC400write_TM(_nkusb, mid, cid, (fadc->TM(j)>>1)&1, fadc->TM(j)&1);
            _kadc.NFADC400write_PCT(_nkusb, mid, cid, fadc->PCT(j));
            _kadc.NFADC400write_PCI(_nkusb, mid, cid, fadc->PCI(j));
            _kadc.NFADC400write_PWT(_nkusb, mid, cid, (float)fadc->PWT(j));
        }
        
        // 설정값 확인 출력
        ShowRegisters(fadc);
    }
    
    // 데이터 버퍼 초기화
    if (_runinfo->GetNFadcBD() > 0) _recordlength = _runinfo->GetFadcBD(0)->RL() * 256; 
    else _recordlength = 1024;
    _neventperdump = _runinfo->GetFadcNDumpedEvent(); 
    _rawdata = new TClonesArray("RawChannel", 100);
}

int main(int argc, char ** argv) {
    int opt;
    // [기본값] Quick Test Mode
    double presetTime = 10.0;
    int presetNEvt = 0;
    string config_file = "configs/daq.config";
    string output_file = "data/test_run.root";
    
    bool user_config = false;
    bool user_output = false;

    while((opt = getopt(argc, argv, "f:o:n:t:")) != -1) {
        switch(opt) {
            case 'f': config_file = optarg; user_config = true; break;
            case 'o': output_file = optarg; user_output = true; break;
            case 'n': presetNEvt = atoi(optarg); presetTime = 0; break;
            case 't': presetTime = atof(optarg); presetNEvt = 0; break;
        }
    }

    if (!user_config && !user_output && presetNEvt == 0) {
        cout << "\n========================================================" << endl;
        cout << " No arguments provided. Running Quick Test Mode!" << endl;
        cout << " Default Config : " << config_file << endl;
        cout << " Default Output : " << output_file << endl;
        cout << " Default Time   : " << presetTime << " seconds" << endl;
        cout << "========================================================\n" << endl;
    }

    Initialize();
    Configuration_byTicket(config_file.c_str());
    _outputfilename = output_file.c_str();
    if(!_doneconfiguration) { _log.Print(ELog::ERROR, "Config failed."); return 1; }
    
    _socket = new TSocket("localhost", 9090);
    if(!_socket->IsValid()) { _havedisplay = false; delete _socket; _socket=NULL; }
    
    PrepareOutputEnvironment(config_file, output_file);

    RunInitialize();
    opennewfile(_outputfilename);
    signal(SIGINT, signal_handler);

    int nbd = _runinfo->GetNFadcBD();
    for(int i=0; i<nbd; i++) {
        unsigned long mid = _runinfo->GetFadcBD(i)->MID();
        _kadc.NFADC400startL(_nkusb, mid);
        _kadc.NFADC400startH(_nkusb, mid);
    }
    
    _log.Print(ELog::INFO, "DAQ Started...");
    _is_running = true;
    
    long bufSize = _recordlength * _neventperdump + 1024;
    char * raw_buffer = new char[bufSize];
    long tagBufSize = _neventperdump * 16 + 1024; 
    char * tag_buffer = new char[tagBufSize];

    int nevt = 0;
    int bufnum = 0;
    TStopwatch sw;
    sw.Start();
    
    while(_is_running) {
        int isFill = 0;
        unsigned long mid0 = _runinfo->GetFadcBD(0)->MID();
        
        while (_is_running) {
            if (bufnum == 0) isFill = !(_kadc.NFADC400read_RunL(_nkusb, mid0));
            else             isFill = !(_kadc.NFADC400read_RunH(_nkusb, mid0));
            if (isFill) break;
            if (presetTime > 0 && sw.RealTime() >= presetTime) { _is_running = false; break; }
        }
        if (!_is_running) break;

        for(int i=0; i<nbd; i++) {
            FadcBD * bd = _runinfo->GetFadcBD(i);
            unsigned long mid = bd->MID();
            int nch = bd->NCHANNEL();
            
             _kadc.NFADC400dump_TAG(_nkusb, mid, bd->CID(0)+1, bd->RL(), bufnum, tag_buffer);
            
            vector<vector<RawChannel*>> event_channels(nch, vector<RawChannel*>(_neventperdump));
            
            for(int j=0; j<nch; j++) {
                int cid = bd->CID(j) + 1;
                _kadc.NFADC400dump_BUFFER(_nkusb, mid, cid, _recordlength, bufnum, raw_buffer);
                
                int nPoints = _recordlength / 2;
                for(unsigned int e=0; e<_neventperdump; e++) {
                    RawChannel * chObj = new RawChannel(bd->CID(j), nPoints);
                    unsigned char* ptr = (unsigned char*)raw_buffer + e * _recordlength;
                    
                    for(int k=0; k<nPoints; k++) {
                        unsigned short val = (unsigned short)((ptr[2*k+1] << 8) | ptr[2*k]);
                        chObj->AddSample(val & 0x3FF); 
                    }
                    event_channels[j][e] = chObj;
                }
            }

            for(unsigned int e=0; e<_neventperdump; e++) {
                _rawdata->Delete();
                int pid = 0;
                
                for(int j=0; j<nch; j++) {
                    RawChannel* src = event_channels[j][e];
                    RawChannel* dest = new((*_rawdata)[pid++]) RawChannel(src->ChannelId(), src->GetNPoints());
                    const vector<unsigned short>& w = src->GetWaveform();
                    for(auto s : w) dest->AddSample(s);
                    delete src;
                }
                
                unsigned char* tptr = (unsigned char*)tag_buffer + e * 8; 
                unsigned long long t1 = tptr[0];
                unsigned long long t2 = tptr[1];
                unsigned long long t3 = tptr[2];
                unsigned long long t4 = tptr[3];
                unsigned long long t5 = tptr[4];
                unsigned long long t6 = tptr[5];
                _ttime = t1 | (t2<<8) | (t3<<16) | (t4<<24) | (t5<<32) | (t6<<40);
                
                _tpattern = tptr[6]; 

                _event_id = nevt + e;
                _rawdatatree->Fill();
                
                if(e == 0 && _havedisplay && _socket) {
                    TMessage mess(kMESS_OBJECT);
                    mess.WriteObject(_rawdata);
                    _socket->Send(mess);
                }
            }
        }
        
        nevt += _neventperdump;
        if (presetNEvt > 0 && nevt >= presetNEvt) { _is_running = false; break; }
        if (presetTime > 0 && sw.RealTime() >= presetTime) { _is_running = false; break; }
        
        if (bufnum == 0) {
            bufnum = 1;
            for(int i=0; i<nbd; i++) _kadc.NFADC400startL(_nkusb, _runinfo->GetFadcBD(i)->MID());
        } else {
            bufnum = 0;
            for(int i=0; i<nbd; i++) _kadc.NFADC400startH(_nkusb, _runinfo->GetFadcBD(i)->MID());
        }
        
        printf("\rEvents: %d | Time: %.1f s", nevt, sw.RealTime());
        fflush(stdout);
    }
    
    printf("\n");
    for(int i=0; i<nbd; i++) {
        _kadc.NFADC400stopL(_nkusb, _runinfo->GetFadcBD(i)->MID());
        _kadc.NFADC400stopH(_nkusb, _runinfo->GetFadcBD(i)->MID());
    }

    _log.Print(ELog::INFO, "Run Finished.");
    delete [] raw_buffer;
    delete [] tag_buffer;
    _outputfile->Write();
    _outputfile->Close();
    
    return 0;
}
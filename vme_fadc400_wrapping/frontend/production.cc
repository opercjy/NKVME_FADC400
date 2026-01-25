#include "production.hh"

#include <iostream>
#include <cmath>
#include <unistd.h>
#include <vector>

// ROOT Headers
#include "TString.h"
#include "TApplication.h"
#include "TStyle.h"
#include "TSystem.h"
#include "TF1.h"
#include "TLine.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TCanvas.h"

using namespace std;

// =============================================================================
// Constructor & Destructor
// =============================================================================
ProductionAnalyzer::ProductionAnalyzer(const std::string& filename) 
    : _filename(filename), _infile(NULL), _intree(NULL), _rawdata(NULL), 
      _isValid(false), _nEntries(0), 
      _canvasEvent(NULL), _canvasCumul(NULL)
{
    // Initialize pointers
    for(int i=0; i<4; i++) {
        _histWave[i] = NULL;
        _histCumul[i] = NULL;
    }

    // Open File
    _infile = new TFile(_filename.c_str(), "READ");
    if (!_infile || _infile->IsZombie()) {
        cerr << "Error: Cannot open file " << _filename << endl;
        return;
    }

    // Get Tree
    _intree = (TTree*)_infile->Get("FADC");
    if (!_intree) {
        cerr << "Error: TTree 'FADC' not found in file." << endl;
        return;
    }

    // Set Branch Address
    _rawdata = new TClonesArray("RawChannel");
    _intree->SetBranchAddress("rawdata", &_rawdata);
    
    // Check for Time/Pattern branches (Compatibility check)
    if(_intree->GetBranch("ttime")) _intree->SetBranchAddress("ttime", &_inTTime);
    else _inTTime = 0;

    _nEntries = _intree->GetEntries();
    _isValid = true;

    // Initial Polarity Check (using first event)
    if (_nEntries > 0) {
        _intree->GetEntry(0);
        if (_rawdata->GetEntries() > 0) {
            RawChannel* ch = (RawChannel*)_rawdata->At(0);
            const vector<unsigned short>& w = ch->GetWaveform();
            
            // Simple check: compare min/max deviation from rough baseline
            if (w.size() > 50) {
                double sum = 0; for(int i=0; i<20; i++) sum += w[i];
                double bsl = sum / 20.0;
                double minV = 99999, maxV = -99999;
                for(auto v : w) {
                    if(v < minV) minV = v;
                    if(v > maxV) maxV = v;
                }
                // If negative dip is deeper than positive spike -> Negative (0)
                _polarity = (abs(bsl - minV) > abs(maxV - bsl)) ? 0 : 1;
            } else {
                _polarity = 0; // Default Negative
            }
        }
    }
    cout << ">>> Opened " << _filename << " (" << _nEntries << " events). Polarity: " << (_polarity ? "Pos" : "Neg") << endl;
}

ProductionAnalyzer::~ProductionAnalyzer() {
    if (_infile) {
        _infile->Close();
        delete _infile;
    }
    // Note: ROOT Canvases/Hists are usually handled by ROOT's object ownership system.
    // Explicit deletion might cause double-free if TApplication closes them first.
}

// =============================================================================
// Analysis Logic (Restored from Original 400MHz Code)
// =============================================================================
void ProductionAnalyzer::AnalyzeWaveform(const vector<unsigned short>& wave, double &bsl, double &amp, double &time, double &charge) {
    int ndp = wave.size();
    if (ndp < 200) { // Too short to analyze
        bsl = 0; amp = 0; time = 0; charge = 0;
        return;
    }

    // [Original Logic Parameters]
    int ped_s = 20;   // Pedestal Start
    int ped_e = 150;  // Pedestal End
    int sig_s = 160;  // Signal Search Start
    // int sig_e = ndp; // Signal Search End

    // 1. Pedestal Calculation (Mean & RMS)
    double pedSum = 0;
    double ped2Sum = 0;
    int nPed = 0;

    for(int i = ped_s; i <= ped_e && i < ndp; i++){
        double val = (double)wave[i];
        pedSum  += val;
        ped2Sum += val * val;
        nPed++;
    }

    double pedMean = (nPed > 0) ? pedSum / nPed : 0;
    double ped2Mean = (nPed > 0) ? ped2Sum / nPed : 0;
    double pedVar = ped2Mean - pedMean * pedMean;
    double pedRms = (pedVar > 0) ? sqrt(pedVar) : 0;

    // 2. Signal Analysis
    double qTotal = 0;
    double fmax = 0;
    int fmaxx = 0;
    double tStart = 0;
    bool foundStart = false;

    // Threshold for timing: 3 * RMS
    double threshold = 3.0 * pedRms;
    if (threshold < 3.0) threshold = 3.0; // Minimum threshold

    for(int i = sig_s; i < ndp; i++){
        double val = (double)wave[i];
        double signal = 0;

        // Calculate signal relative to pedestal based on polarity
        if (_polarity == 0) signal = pedMean - val; // Negative Pulse
        else                signal = val - pedMean; // Positive Pulse

        // Accumulate Charge
        qTotal += signal;

        // Find Peak
        if (signal > fmax) {
            fmax = signal;
            fmaxx = i;
        }

        // Find Start Time (Leading Edge)
        if (!foundStart && signal >= threshold) {
            tStart = i * 2.5; // 400MHz = 2.5ns per sample
            foundStart = true;
        }
    }

    // Set Outputs
    bsl = pedMean;
    amp = fmax;
    // If no trigger found, use peak time
    time = (foundStart) ? tStart : (fmaxx * 2.5);
    charge = qTotal;
}

// =============================================================================
// Batch Mode (Write to File)
// =============================================================================
void ProductionAnalyzer::RunBatch(bool saveWaveform) {
    if (!_isValid) return;

    TString outName = _filename;
    outName.ReplaceAll(".root", "_prod.root");
    
    TFile* fout = new TFile(outName, "RECREATE");
    TTree* trout = new TTree("PROD", "Processed FADC Data");

    // Output Variables
    int nch;
    int cid[4];
    double bsl[4], amp[4], time[4], charge[4];
    unsigned long long outTime;

    trout->Branch("nch", &nch, "nch/I");
    trout->Branch("cid", cid, "cid[nch]/I");
    trout->Branch("bsl", bsl, "bsl[nch]/D");
    trout->Branch("amp", amp, "amp[nch]/D");
    trout->Branch("time", time, "time[nch]/D");
    trout->Branch("charge", charge, "charge[nch]/D");
    trout->Branch("ttime", &outTime, "ttime/l");

    TClonesArray* outWaveforms = nullptr;
    if (saveWaveform) {
        outWaveforms = new TClonesArray("RawChannel", 100);
        trout->Branch("waveforms", &outWaveforms);
    }

    cout << ">>> Batch Processing " << _nEntries << " events..." << endl;

    for(int i=0; i<_nEntries; i++) {
        _intree->GetEntry(i);
        outTime = _inTTime; // Copy timestamp
        
        if (saveWaveform && outWaveforms) outWaveforms->Clear();

        nch = _rawdata->GetEntries();
        if (nch > 4) nch = 4; // Safety limit

        for(int j=0; j<nch; j++) {
            RawChannel* ch = (RawChannel*)_rawdata->At(j);
            cid[j] = ch->ChannelId();
            
            AnalyzeWaveform(ch->GetWaveform(), bsl[j], amp[j], time[j], charge[j]);

            // Save Raw Waveform if requested
            if (saveWaveform && outWaveforms) {
                RawChannel* newCh = new((*outWaveforms)[j]) RawChannel(ch->ChannelId(), ch->GetNPoints());
                const vector<unsigned short>& wav = ch->GetWaveform();
                for(unsigned short val : wav) newCh->AddSample(val);
            }
        }

        trout->Fill();

        if (i % 1000 == 0) {
            printf("\rProgress: %d / %d (%.1f%%)", i, _nEntries, (float)i/_nEntries*100);
            fflush(stdout);
        }
    }

    cout << "\n>>> Saving to " << outName << endl;
    trout->Write();
    
    // Attempt to copy RunInfo
    TObject* rinfo = _infile->Get("RunInfo");
    if(rinfo) rinfo->Write();

    fout->Close();
    delete fout;
}

// =============================================================================
// Interactive Mode (Visualization)
// =============================================================================
void ProductionAnalyzer::ShowEvent(int entry) {
    if (!_canvasEvent) {
        _canvasEvent = new TCanvas("c_event", "Waveform Viewer", 1200, 800);
        _canvasEvent->Divide(2, 2);
    }

    _intree->GetEntry(entry);
    int nch = _rawdata->GetEntries();

    for(int i=0; i<4; i++) {
        _canvasEvent->cd(i+1);
        gPad->Clear();
        gPad->SetGrid();

        if (i < nch) {
            RawChannel* ch = (RawChannel*)_rawdata->At(i);
            const vector<unsigned short>& wav = ch->GetWaveform();
            int ns = wav.size();

            // Clear previous histogram
            if (_histWave[i]) delete _histWave[i];
            _histWave[i] = new TH1F(Form("h_ev%d_ch%d", entry, i), 
                                    Form("Event %d - Channel %d;Time (ns);ADC", entry, ch->ChannelId()), 
                                    ns, 0, ns * 2.5); // 2.5ns steps

            // Fill Histogram
            for(int k=0; k<ns; k++) {
                _histWave[i]->SetBinContent(k+1, wav[k]);
            }
            
            // --- Original Logic: Pedestal Fit & Display ---
            // Fit pol0 (constant) to the pre-trigger region (20~150)
            _histWave[i]->Fit("pol0", "Q0", "", 20 * 2.5, 150 * 2.5); 
            TF1* func = _histWave[i]->GetFunction("pol0");
            double ped = func ? func->GetParameter(0) : 0;

            // Draw Waveform
            _histWave[i]->SetLineColor(kBlue + 1);
            _histWave[i]->Draw("HIST");
            
            // Draw Pedestal Line (Red Dashed)
            TLine* line = new TLine(0, ped, ns * 2.5, ped);
            line->SetLineColor(kRed);
            line->SetLineStyle(2);
            line->SetLineWidth(2);
            line->Draw();

            // Calculate analysis values for display
            double bsl, amp, time, charge;
            AnalyzeWaveform(wav, bsl, amp, time, charge);
            
            // Draw Start Time Line (Green)
            if (time > 0) {
                TLine* tline = new TLine(time, 0, time, 4096);
                tline->SetLineColor(kGreen + 2);
                tline->SetLineStyle(1);
                tline->Draw();
            }
        }
    }
    _canvasEvent->Update();
}

void ProductionAnalyzer::ShowCumulative() {
    if (!_canvasCumul) {
        _canvasCumul = new TCanvas("c_cumul", "Cumulative Waveforms", 1200, 800);
        _canvasCumul->Divide(2, 2);
    }

    cout << ">>> Generating cumulative plots (this may take a while)..." << endl;

    _intree->GetEntry(0);
    int ns = 512;
    if (_rawdata->GetEntries() > 0) {
        RawChannel* ch = (RawChannel*)_rawdata->At(0);
        ns = ch->GetNPoints();
    }

    for(int i=0; i<4; i++) {
        if (_histCumul[i]) delete _histCumul[i];
        // [FIX] Y-axis range: 0 ~ 1024 (10-bit)
        _histCumul[i] = new TH2F(Form("h_cumul_ch%d", i), 
                                 Form("Cumulative Ch %d;Time (ns);ADC", i), 
                                 ns, 0, ns * 2.5, 
                                 256, 0, 1024); 
    }

    // Fill Histograms
    for(int i=0; i<_nEntries; i++) {
        _intree->GetEntry(i);
        int nch = _rawdata->GetEntries();
        for(int j=0; j<nch && j<4; j++) {
            RawChannel* ch = (RawChannel*)_rawdata->At(j);
            const vector<unsigned short>& wav = ch->GetWaveform();
            for(size_t k=0; k<wav.size(); k++) {
                _histCumul[j]->Fill(k * 2.5, wav[k]);
            }
        }
        if (i % 2000 == 0) { printf("\rProcessing... %d%%", (int)((float)i/_nEntries*100)); fflush(stdout); }
    }
    cout << "\nDone!" << endl;

    // Draw
    for(int i=0; i<4; i++) {
        _canvasCumul->cd(i+1);
        gPad->SetLogz(); // Log scale for frequency
        gPad->SetGrid();
        _histCumul[i]->Draw("COLZ");
    }
    _canvasCumul->Update();
}

void ProductionAnalyzer::RunInteractive() {
    if (!_isValid) return;

    int currEntry = 0;
    char cmd;
    
    cout << "==========================================================" << endl;
    cout << " Interactive Mode Started" << endl;
    cout << " [n] Next Event   [p] Prev Event   [j] Jump to Event" << endl;
    cout << " [t] Total (Cumulative) Plot       [q] Quit" << endl;
    cout << "==========================================================" << endl;

    ShowEvent(currEntry);

    while(true) {
        cout << "\nCommand (Event " << currEntry << "/" << _nEntries-1 << ") > ";
        cin >> cmd;

        if (cmd == 'n') {
            if (currEntry < _nEntries - 1) currEntry++;
            ShowEvent(currEntry);
        }
        else if (cmd == 'p') {
            if (currEntry > 0) currEntry--;
            ShowEvent(currEntry);
        }
        else if (cmd == 'j') {
            int dest;
            cout << "Jump to event number: ";
            cin >> dest;
            if (dest >= 0 && dest < _nEntries) {
                currEntry = dest;
                ShowEvent(currEntry);
            } else {
                cout << "Invalid event number!" << endl;
            }
        }
        else if (cmd == 't') {
            ShowCumulative();
        }
        else if (cmd == 'q') {
            break;
        }
        else {
            cout << "Unknown command. Options: n, p, j, t, q" << endl;
        }
    }
}

// =============================================================================
// Main Function
// =============================================================================
void PrintUsage(const char* progName) {
    cout << "Usage: " << progName << " -i <input.root> [-w] [-d]" << endl;
    cout << "  -i <file> : Input ROOT file from DAQ" << endl;
    cout << "  -w        : Write Mode (Process and save to _prod.root)" << endl;
    cout << "              (Saves Waveforms, Charge, Amp, Time)" << endl;
    cout << "  -d        : Display Mode (Interactive Viewer)" << endl;
}

int main(int argc, char** argv) {
    string inputFile;
    bool modeWrite = false;
    bool modeDisplay = false;

    int opt;
    while ((opt = getopt(argc, argv, "i:wd")) != -1) {
        switch (opt) {
            case 'i': inputFile = optarg; break;
            case 'w': modeWrite = true; break;
            case 'd': modeDisplay = true; break;
            default: PrintUsage(argv[0]); return 1;
        }
    }

    if (inputFile.empty()) {
        cerr << "Error: Input file is required (-i)." << endl;
        PrintUsage(argv[0]);
        return 1;
    }

    if (modeWrite && modeDisplay) {
        cerr << "Error: Select either -w (Write) or -d (Display), not both." << endl;
        return 1;
    }

    if (!modeWrite && !modeDisplay) {
        cerr << "Error: No mode selected. Use -w or -d." << endl;
        PrintUsage(argv[0]);
        return 1;
    }

    // --- ROOT Application for Display Mode ---
    TApplication* app = nullptr;
    if (modeDisplay) {
        app = new TApplication("App", &argc, argv);
    }

    // --- Create Analyzer Instance ---
    ProductionAnalyzer analyzer(inputFile);
    if (!analyzer.IsValid()) return 1;

    // --- Run Selected Mode ---
    if (modeWrite) {
        // saveWaveform = true (Always save waveforms for now)
        analyzer.RunBatch(true);
    }
    else if (modeDisplay) {
        analyzer.RunInteractive();
        if (app) app->Terminate();
    }

    return 0;
}
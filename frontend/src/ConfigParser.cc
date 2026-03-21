#include "ConfigParser.hh"
#include "ELog.hh"
#include "TObjString.h"
#include "TObjArray.h"
#include <fstream>
#include <string>
#include <algorithm> 
#include <cstdlib>

bool ConfigParser::Parse(const char* filename, RunInfo* runinfo) {
    std::ifstream ticket(filename);
    if (!ticket.is_open()) {
        ELog::Print(ELog::ERROR, Form("Cannot open config file: %s", filename));
        return false;
    }

    ELog::Print(ELog::INFO, Form("Loading Configuration: %s", filename));

    std::string line;
    int line_num = 0; 

    while(std::getline(ticket, line)) {
        line_num++;
        if(line == "END") break;
        
        TString ts(line);
        if(ts.BeginsWith("#") || ts.IsWhitespace()) continue;

        TObjArray * token = ts.Tokenize(" \t");
        if(token->GetEntriesFast() == 0) { 
            delete token; 
            continue; 
        }

        TString key = ((TObjString*)token->At(0))->String();
        key.ToUpper(); 

        if (key == "RUNNUM") {
            if (token->GetEntriesFast() >= 2)
                runinfo->SetRunNumber(((TObjString*)token->At(1))->String().Atoi());
        }
        else if (key == "RUNTYP") {
            if (token->GetEntriesFast() >= 2)
                runinfo->SetRunType(((TObjString*)token->At(1))->String().Data());
        }
        else if (key == "RUNDSC") {
            if (token->GetEntriesFast() >= 2) {
                TString desc = "";
                for(int k=1; k<token->GetEntriesFast(); k++) {
                    desc += ((TObjString*)token->At(k))->String() + " ";
                }
                desc = desc.Strip(TString::kBoth, ' '); 
                desc = desc.Strip(TString::kBoth, '\"'); 
                desc = desc.Strip(TString::kBoth, ' '); 
                runinfo->SetRunDesc(desc.Data());
            }
        }
        else if (key == "SHIFT") {
            if (token->GetEntriesFast() >= 2) {
                TString shift = "";
                for(int k=1; k<token->GetEntriesFast(); k++) {
                    shift += ((TObjString*)token->At(k))->String() + " ";
                }
                shift = shift.Strip(TString::kBoth, ' ');
                shift = shift.Strip(TString::kBoth, '\"');
                shift = shift.Strip(TString::kBoth, ' ');
                runinfo->SetShift(shift.Data()); 
            }
        }
        else if (key == "FADC") {
            if (token->GetEntriesFast() < 3) {
                ELog::Print(ELog::ERROR, Form("Line %d: FADC requires MID and NCH.", line_num));
            } else {
                int mid = ((TObjString*)token->At(1))->String().Atoi();
                int nch = ((TObjString*)token->At(2))->String().Atoi();
                if (nch < 0 || nch > 4) {
                    ELog::Print(ELog::WARNING, Form("Line %d: FADC NCH out of bounds. Forced to 4.", line_num));
                    nch = 4;
                }
                FadcBD * bd = new FadcBD(mid);
                bd->SetNCH(nch);
                for(int i=0; i<nch; i++) {
                    if(3+i < token->GetEntriesFast())
                        bd->SetCID(i, ((TObjString*)token->At(3+i))->String().Atoi());
                }
                runinfo->AddFadcBD(bd);
            }
        }
        else if (key == "NDP") {
            if (token->GetEntriesFast() >= 3) {
                int mid = ((TObjString*)token->At(1))->String().Atoi();
                FadcBD* bd = runinfo->GetFadcBD(mid);
                if(bd) {
                    int ndp = ((TObjString*)token->At(2))->String().Atoi();
                    if (ndp <= 0 || ndp > 32768) { 
                        ELog::Print(ELog::WARNING, Form("Line %d: NDP value invalid. Keeping default.", line_num));
                    } else {
                        bd->SetNDP(ndp);
                    }
                }
            }
        }
        else if (key == "TLT") {
            if (token->GetEntriesFast() >= 3) {
                int mid = ((TObjString*)token->At(1))->String().Atoi();
                TString val = ((TObjString*)token->At(2))->String();
                val.ToUpper(); 
                
                int code = 0xFFFE;
                if (val == "OR") code = 0xFFFE;
                else if (val == "AND") code = 0x8000;
                else if (val.BeginsWith("0X")) code = strtoul(val.Data(), NULL, 16);
                else code = val.Atoi();
                
                FadcBD* bd = runinfo->GetFadcBD(mid);
                if(bd) bd->SetTLT(code);
            }
        }
        else if (key == "THR" || key == "POL" || key == "DLY" || 
                 key == "DACOFF" || key == "DT" || key == "CW" || 
                 key == "TM" || key == "PCT" || key == "PCI" || key == "PWT") {
            
            if (token->GetEntriesFast() < 3) continue;
            int mid = ((TObjString*)token->At(1))->String().Atoi();
            FadcBD* bd = runinfo->GetFadcBD(mid);
            if (!bd) continue;
            
            int input_count = token->GetEntriesFast() - 2;
            int safe_channels = std::min(input_count, 4);

            if (key == "THR") {
                for (int i = 0; i < safe_channels; i++) {
                    int val = ((TObjString*)token->At(2 + i))->String().Atoi();
                    if (val < 0) continue;
                    bd->SetTHR(i, val); 
                }
            } 
            else if (key == "POL") {
                for (int i = 0; i < safe_channels; i++) {
                    int val = ((TObjString*)token->At(2 + i))->String().Atoi();
                    if (val != 0 && val != 1) continue;
                    bd->SetPOL(i, val);
                }
            } 
            else if (key == "DLY") {
                for (int i = 0; i < safe_channels; i++) {
                    int val = ((TObjString*)token->At(2 + i))->String().Atoi();
                    if (val < 0 || val > 1000) continue;
                    bd->SetDLY(i, val);
                }
            }
            // [수정 완료] 주석 해제하여 완벽하게 하드웨어 값 적용
            else if (key == "DACOFF") {
                for (int i = 0; i < safe_channels; i++) {
                    int val = ((TObjString*)token->At(2 + i))->String().Atoi();
                    if (val < 0 || val > 4095) continue;
                    bd->SetDACOFF(i, val); 
                }
            }
            else if (key == "DT") {
                for (int i = 0; i < safe_channels; i++) {
                    int val = ((TObjString*)token->At(2 + i))->String().Atoi();
                    if (val < 0) continue;
                    bd->SetDT(i, val);
                }
            }
            else if (key == "CW") {
                for (int i = 0; i < safe_channels; i++) {
                    int val = ((TObjString*)token->At(2 + i))->String().Atoi();
                    if (val < 0) continue;
                    bd->SetCW(i, val);
                }
            }
            else if (key == "TM") {
                for (int i = 0; i < safe_channels; i++) {
                    int val = ((TObjString*)token->At(2 + i))->String().Atoi();
                    if (val < 0 || val > 3) continue;
                    bd->SetTM(i, val);
                }
            }
            else if (key == "PCT") {
                for (int i = 0; i < safe_channels; i++) {
                    int val = ((TObjString*)token->At(2 + i))->String().Atoi();
                    if (val < 1) continue;
                    bd->SetPCT(i, val);
                }
            }
            else if (key == "PCI") {
                for (int i = 0; i < safe_channels; i++) {
                    int val = ((TObjString*)token->At(2 + i))->String().Atoi();
                    if (val < 0) continue;
                    bd->SetPCI(i, val);
                }
            }
            else if (key == "PWT") {
                for (int i = 0; i < safe_channels; i++) {
                    int val = ((TObjString*)token->At(2 + i))->String().Atoi();
                    if (val < 0) continue;
                    bd->SetPWT(i, val);
                }
            }
        }
        delete token;
    }
    ticket.close();
    return true;
}
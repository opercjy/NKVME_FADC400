#ifndef frontend_hh
#define frontend_hh

// -----------------------------------------------------------------------------
// System Headers
// -----------------------------------------------------------------------------
#include <vector>
#include <string>

// -----------------------------------------------------------------------------
// ROOT Headers
// -----------------------------------------------------------------------------
#include "TTree.h"
#include "TFile.h"
#include "TSocket.h"
#include "TClonesArray.h"
#include "THashTable.h"
#include "TString.h"
#include "TNamed.h"

// -----------------------------------------------------------------------------
// Hardware & Custom Headers
// -----------------------------------------------------------------------------
#include "NoticeNFADC400.h"
#include "Notice6UVME.h"
#include "RunInfo.hh"
#include "ELog.hh"

// -----------------------------------------------------------------------------
// Helper Classes
// -----------------------------------------------------------------------------

// [Trigger Lookup Node]
class Node : public TNamed {
public:
    Node() : TNamed(), _code(0) {} 
    
    Node(const char* name, unsigned int code) 
        : TNamed(name, "Trigger Node"), _code(code) 
    { }

    unsigned int GetCode() const { return _code; }

private:
    unsigned int _code;
    // ClassDef(Node, 1)  <-- [삭제됨] Dictionary 없이 빌드하기 위해 제거
};

// -----------------------------------------------------------------------------
// Global Variables (Extern Declarations)
// -----------------------------------------------------------------------------
extern NK6UVME    _kvme;
extern NKNFADC400 _kadc;
extern int        _nkusb;

extern RunInfo * _runinfo;
extern ELog       _log;

extern TFile * _outputfile;
extern TTree * _rawdatatree;
extern TClonesArray * _rawdata;

extern TObjArray * _outputfilelist;
extern const char * _outputfilename;
extern int          _noutputfile;
extern bool         _doneconfiguration;

extern TSocket * _socket;
extern bool      _havedisplay;

extern unsigned long _neventperdump;
extern unsigned long _recordlength;

extern THashTable * _triggerlookup;

// -----------------------------------------------------------------------------
// Function Prototypes
// -----------------------------------------------------------------------------
void Initialize();
void Configuration_byTicket(const char * filename);
void RunInitialize();
bool opennewfile(const char * fname);

#endif
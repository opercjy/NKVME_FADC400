#include "ELog.hh"
#include <iostream>
#include <ctime>

using namespace std;

ELog::ELog() {}
ELog::~ELog() {}

void ELog::Print(ELogType type, TString msg)
{
  time_t rawtime;
  struct tm * timeinfo;
  char buffer[80];

  time(&rawtime);
  timeinfo = localtime(&rawtime);
  strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", timeinfo);

  cout << "[" << buffer << "] ";
  
  switch(type) {
      case INFO:    cout << "[INFO]    "; break;
      case WARNING: cout << "[WARNING] "; break;
      case ERROR:   cout << "[ERROR]   "; break;
  }
  
  cout << msg << endl;
}
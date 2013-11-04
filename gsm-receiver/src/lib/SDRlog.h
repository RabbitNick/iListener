#ifndef SDRLOG_H
#define SDRLOG_H

#include <fstream>

using namespace std;

class SDRLog
{
  public:
    SDRLog(char* filename);
    ~SDRLog();
    void Write(const char* logline, ...);
  private:
    ofstream m_stream;
};

#endif



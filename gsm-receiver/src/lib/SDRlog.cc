
#include <stdarg.h>
#include "SDRlog.h"
#include <time.h>

SDRLog::SDRLog(char* filename)
{
  m_stream.open(filename);
}

void SDRLog::Write(const char* logline, ...)
{
  va_list argList;
  char cbuffer[1024];
  va_start(argList, logline);
  vsnprintf(cbuffer, 1024, logline, argList);
  va_end(argList);

  char buff[100];
  time_t now = time(0);
  struct tm *sTm;
  sTm = localtime(&now);//gmtime(&now);
  strftime(buff, sizeof(buff), "%Y-%m-%d %H:%M:%S", sTm);
  
  m_stream << buff;
  m_stream << ": ";
  m_stream << cbuffer << endl;
}

SDRLog::~SDRLog()
{
  m_stream.close();
}



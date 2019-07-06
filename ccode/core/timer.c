#include <windows.h>

double getPerformanceTime() 
{
  static LARGE_INTEGER freq;
  static init_freq = 1;
  LARGE_INTEGER time;
  
  if (init_freq) {
    QueryPerformanceFrequency(&freq);
    init_freq = 0;
  }
  
  
  QueryPerformanceCounter(&time);
  
  return ((double)time.QuadPart / (double)freq.QuadPart)*1000.0;
}

double doSleep(double time_ms) {
  Sleep((DWORD)time_ms);
}

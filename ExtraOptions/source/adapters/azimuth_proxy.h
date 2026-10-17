#ifndef FH2_AZIMUTH_PROXY_H
#define FH2_AZIMUTH_PROXY_H
#include "extra_options.h"
HMODULE WINAPI azimuth_load_library(LPCWSTR path, HANDLE file, DWORD flags);
void azimuth_proxy_start(void *caller);
#endif

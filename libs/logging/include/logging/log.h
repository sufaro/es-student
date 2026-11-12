#pragma once
#include <stdio.h>
#ifndef LOG_LEVEL
#define LOG_LEVEL 2
#endif
#define LOG_ERR(...) do { if (LOG_LEVEL>=1) { printf("err %s:%d ",__func__,__LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)
#define LOG_INF(...) do { if (LOG_LEVEL>=2) { printf("inf %s:%d ",__func__,__LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)
#define LOG_DBG(...) do { if (LOG_LEVEL>=3) { printf("dbg %s:%d ",__func__,__LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

#ifdef _WIN32
#include <windows.h>
typedef HANDLE thread_t;
#else
#include <pthread.h>
typedef pthread_t thread_t;
#endif

#include "worker.h"

thread_t start_worker(WorkerContext *ctx);
void join_worker(thread_t t);

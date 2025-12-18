#include "worker_platform.h"
#include "worker.h"

#ifdef _WIN32
#include <windows.h>

static DWORD WINAPI worker_trampoline(LPVOID arg)
{
    worker_entry(arg);
    return 0;
}

thread_t start_worker(WorkerContext *ctx)
{
    return CreateThread(
        NULL, // default security
        0,    // default stack size
        worker_trampoline,
        ctx,
        0, // run immediately
        NULL);
}

void join_worker(thread_t t)
{
    WaitForSingleObject(t, INFINITE);
    CloseHandle(t);
    printf("Thread exited!\n");
}
#else
#include <pthread.h>

thread_t start_worker(WorkerContext *ctx)
{
    pthread_t t;
    pthread_create(&t, NULL, worker_entry, ctx);
    return t;
}

void join_worker(thread_t t)
{
    pthread_join(t, NULL);
    printf("Thread exited!\n");
}

#endif

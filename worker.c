#include "worker.h"
#include "calculations.h"

int worker_entry(void *arg)
{
    WorkerContext *ctx = (WorkerContext *)arg;

    atomic_store(&ctx->state, WORKER_RUNNING);

    int result = SuggestBestBaseAndAmmo_RegionAware(
        ctx->list,
        ctx->target,
        &ctx->result_base,
        &ctx->result_ammo,
        ctx);

    if (result == -1)
    {
        atomic_store(&ctx->state, WORKER_CANCELLED);
    }
    else if (result == 0)
    {
        atomic_store(&ctx->state, WORKER_FAILED);
    }
    else
    {
        atomic_store(&ctx->state, WORKER_DONE);
    }

    return 0;
}

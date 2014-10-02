#include "dtap.h"
#include "EffectBundle.h"

static ap_wrapper_t *g_ap;
static int dtap_inited = 0;
static dtap_lock_t mutex;

#define EQ_ANDROID 1

static void reg_ap_all()
{
#if EQ_ANDROID
    extern ap_wrapper_t ap_lvm_eq;
    g_ap = &ap_lvm_eq;
#else
    extern ap_wrapper_t ap_private;
    g_ap = &ap_private;
#endif
}

static int dtap_select(dtap_context_t *ctx)
{
    if(!g_ap)
        return -1;
    ctx->wrapper = g_ap;
    return 0;
}

void dtap_global_init()
{
    if(dtap_inited == 1)
        return;
    reg_ap_all();
    dtap_inited = 1;
    return;
}

int dtap_init(dtap_context_t *ctx)
{
    int ret = 0;
    dtap_global_init();
    ret = dtap_select(ctx);
    if(ret < 0)
        goto EXIT;
    ap_wrapper_t *wrapper = ctx->wrapper;
    ret = wrapper->init(ctx);
    dtap_print("dtap_init ok \n");
    ctx->inited = 1;
    dtap_lock_init(&mutex, NULL);
EXIT:
    return ret;
}

int dtap_process(dtap_context_t *ctx, dtap_frame_t *frame)
{

    if(!ctx->inited)
        dtap_init(ctx);
    dtap_lock(&mutex); 
    ap_wrapper_t *wrapper = ctx->wrapper;
    wrapper->process(ctx, frame);
    dtap_unlock(&mutex); 
    return 0;
}

int dtap_update(dtap_context_t *ctx)
{
    dtap_lock(&mutex); 
    ap_wrapper_t *wrapper = ctx->wrapper;
    wrapper->release(ctx);
    ctx->inited = 0;
    dtap_unlock(&mutex); 
    return 0;
}

int dtap_release(dtap_context_t * ctx)
{
    dtap_lock(&mutex); 
    dtap_print("dtap_release ok \n");
    dtap_unlock(&mutex); 
    return 0;
}

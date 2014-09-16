#include "dtap.h"

int dtap_init(dtap_context_t * ctx)
{
    dtap_print("dtap_init ok \n");
    return 0;
}

int dtap_process(dtap_context_t *ctx, dtap_frame_t *frame)
{
    dtap_print("dtap_process ok \n");
    return 0;
}

int dtap_release(dtap_context_t * ctx)
{
    dtap_print("dtap_release ok \n");
    return 0;
}

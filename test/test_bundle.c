// bundle tests

#include "dtap.h"
#include "LVM.h"

int bundle_test()
{
    dtap_context_t ctx;
    memset(&ctx, 0 , sizeof(dtap_context_t));
    int ret = dtap_process(&ctx, NULL);
    dtap_print("ret :%d \n", ret);
    return 0;
}

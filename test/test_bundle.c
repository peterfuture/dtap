// bundle tests

#include "dtap.h"
#include "LVM.h"

int bundle_test()
{
    dtap_context_t ctx;
    int ret = dtap_init(&ctx);
    dtap_print("ret :%d \n", ret);
    return 0;
}

// bundle tests

#include "dtap.h"
#include "LVM.h"

int bundle_test()
{
    LVM_Handle_t h_instance;
    LVM_MemTab_t m_tab;
    LVM_InstParams_t i_para;

    LVM_ReturnStatus_en ret = LVM_GetInstanceHandle(&h_instance, &m_tab, &i_para);

    dtap_print("ret :%d \n", ret);
    return 0;
}

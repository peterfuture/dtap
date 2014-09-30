// bundle tests

#include "dtap.h"
#include "LVM.h"

int bundle_test(char *in, char *out)
{
    dtap_context_t ctx;
    memset(&ctx, 0 , sizeof(dtap_context_t));

    dtap_para_t * ppara = &ctx.para;
    ppara->samplerate = 44100;
    ppara->channels = 2;
    ppara->data_width = 16;

    ppara->type = DTAP_EFFECT_EQ;
    ppara->item = EQ_EFFECT_HEAVYMETAL; 

    int bytes_per_sample = ppara->channels * ppara->data_width /8;
    int PCM_WRITE_SIZE = 10;
    int unit_size = PCM_WRITE_SIZE * ppara->samplerate * bytes_per_sample / 1000;
    unit_size = unit_size - unit_size % (bytes_per_sample *4); // for audio effect 4 samples align

    dtap_frame_t frame;
    frame.in = malloc(unit_size);
    frame.in_size =unit_size;

    int ret = dtap_process(&ctx, &frame);
    dtap_print("ret :%d \n", ret);
    return 0;
}

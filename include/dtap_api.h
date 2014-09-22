#ifndef DTAP_API_H
#define DTAP_API_H

#define MAX_NAME_LEN 1024

enum{
    DTAP_ID_ANDROID = 0x0,
    DTAP_ID_PRIVATE = 0x1
};

enum{
    DTAP_EFFECT_NONE      = 0x0,
    DTAP_EFFECT_CLASSIC   = 0x1,
    DTAP_EFFECT_MAX       = 0x800000
};

struct dtap_context;

typedef struct{
    uint8_t *in_buf;
    uint8_t *out_buf;

    int in_len;
    int out_len;
}dtap_frame_t;

typedef struct{
    int samplerate;
    int channels;
    int bps;
    int effect_id;  // CLASSIC for example
}dtap_para_t;

typedef struct{
    int id;
    char *name;

    int (*init)     (struct dtap_context *ctx);
    int (*process)  (struct dtap_context *ctx, dtap_frame_t *frame);
    int (*config)   (struct dtap_context *ctx);
    int (*release)  (struct dtap_context *ctx);
}ap_wrapper_t;

typedef struct dtap_context{
    dtap_para_t para;   // used to config ap
    char name[MAX_NAME_LEN];
    ap_wrapper_t *wrapper;
    int inited;
    void *ap_priv;
}dtap_context_t;

int dtap_process(dtap_context_t *ctx, dtap_frame_t *frame);
int dtap_reset(dtap_context_t *ctx, dtap_para_t *para);
int dtap_release(dtap_context_t *ctx);

#endif

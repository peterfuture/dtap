#ifndef DT_AP_H

#include "stdio.h"
#include "stdint.h"
#include "stdlib.h"
#include "string.h"


//################################################
#include "pthread.h"
#define dtap_lock_t         pthread_mutex_t
#define dtap_lock_init(x,v) pthread_mutex_init(x,v)
#define dtap_lock(x)        pthread_mutex_lock(x)
#define dtap_unlock(x)      pthread_mutex_unlock(x)
//################################################

//################################################
#define dtap_print printf

//################################################

//################################################
#define dtap_malloc malloc
#define dtap_free   free

//################################################



#define MAX_NAME_LEN 1024
typedef enum{
    DTAP_EFFECT_NONE      = 0x0,
    DTAP_EFFECT_CLASSIC   = 0x1,

    DTAP_EFFECT_MAX       = 0x800000,
};

typedef struct{
    uint8_t *in_buf;
    uint8_t *out_buf;

    int in_len;
    int out_len;
}dtap_frame_t;

typedef struct{
    char name[MAX_NAME_LEN];
    dtap_lock_t mutex;
}dtap_context_t;


int dtap_init(dtap_context_t * ctx);
int dtap_process(dtap_context_t *ctx, dtap_frame_t *frame);
int dtap_release(dtap_context_t *ctx);

#endif

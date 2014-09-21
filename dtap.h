#ifndef DT_AP_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "dtap_api.h"

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

#endif

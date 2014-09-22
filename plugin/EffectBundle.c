/*
 * Copyright (C) 2010-2010 NXP Software
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "Bundle"
#define ARRAY_SIZE(array) (sizeof array / sizeof array[0])
//#define LOG_NDEBUG 0

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "EffectBundle.h"
#include "dtap.h"

#define LVM_ERROR_CHECK(LvmStatus, callingFunc, calledFunc){\
        if (LvmStatus == LVM_NULLADDRESS){\
            printf("\tLVM_ERROR : Parameter error - "\
                    "null pointer returned by %s in %s\n\n\n\n", callingFunc, calledFunc);\
        }\
        if (LvmStatus == LVM_ALIGNMENTERROR){\
            printf("\tLVM_ERROR : Parameter error - "\
                    "bad alignment returned by %s in %s\n\n\n\n", callingFunc, calledFunc);\
        }\
        if (LvmStatus == LVM_INVALIDNUMSAMPLES){\
            printf("\tLVM_ERROR : Parameter error - "\
                    "bad number of samples returned by %s in %s\n\n\n\n", callingFunc, calledFunc);\
        }\
        if (LvmStatus == LVM_OUTOFRANGE){\
            printf("\tLVM_ERROR : Parameter error - "\
                    "out of range returned by %s in %s\n", callingFunc, calledFunc);\
        }\
    }


static inline int16_t clamp16(int32_t sample)
{
    // check overflow for both positive and negative values:
    // all bits above short range must me equal to sign bit
    if ((sample>>15) ^ (sample>>31))
        sample = 0x7FFF ^ (sample>>31);
    return sample;
}

/* local functions */
#define CHECK_ARG(cond) {                     \
    if (!(cond)) {                            \
        printf("\tLVM_ERROR : Invalid argument: "#cond);      \
        return -EINVAL;                       \
    }                                         \
}

/* Effect Library Interface Implementation */

static int samplerate_convert(int rate)
{
    switch(rate)
    {
        case 8000:
            return LVM_FS_8000;
        case 11025:
            return LVM_FS_11025;
        case 12000:
            return LVM_FS_12000;
        case 16000:
            return LVM_FS_16000;
        case 22050:
            return LVM_FS_22050;
        case 24000:
            return LVM_FS_24000;
        case 32000:
            return LVM_FS_32000;
        case 44100:
            return LVM_FS_44100;
        case 48000:
            return LVM_FS_48000;
        default:
            return LVM_FS_48000;
    }

    return LVM_FS_48000;
}


//----------------------------------------------------------------------------
// LvmBundle_init()
//----------------------------------------------------------------------------
// Purpose: Initialize engine with default configuration, creates instance
// with all effects disabled.
//
// Inputs:
//  pContext:   effect engine context
//
// Outputs:
//
//----------------------------------------------------------------------------

int LvmBundle_init(EffectContext *pContext){
    //int status;
    int i;
    printf("\tLvmBundle_init start \n");

#if 0
    pContext->config.inputCfg.accessMode                    = EFFECT_BUFFER_ACCESS_READ;
    pContext->config.inputCfg.channels                      = AUDIO_CHANNEL_OUT_STEREO;
    pContext->config.inputCfg.format                        = AUDIO_FORMAT_PCM_16_BIT;
    pContext->config.inputCfg.samplingRate                  = 44100;
    pContext->config.inputCfg.bufferProvider.getBuffer      = NULL;
    pContext->config.inputCfg.bufferProvider.releaseBuffer  = NULL;
    pContext->config.inputCfg.bufferProvider.cookie         = NULL;
    pContext->config.inputCfg.mask                          = EFFECT_CONFIG_ALL;
    pContext->config.outputCfg.accessMode                   = EFFECT_BUFFER_ACCESS_ACCUMULATE;
    pContext->config.outputCfg.channels                     = AUDIO_CHANNEL_OUT_STEREO;
    pContext->config.outputCfg.format                       = AUDIO_FORMAT_PCM_16_BIT;
    pContext->config.outputCfg.samplingRate                 = 44100;
    pContext->config.outputCfg.bufferProvider.getBuffer     = NULL;
    pContext->config.outputCfg.bufferProvider.releaseBuffer = NULL;
    pContext->config.outputCfg.bufferProvider.cookie        = NULL;
    pContext->config.outputCfg.mask                         = EFFECT_CONFIG_ALL;
#endif
    CHECK_ARG(pContext != NULL);

    if (pContext->pBundledContext->hInstance != NULL){
        printf("\tLvmBundle_init pContext->pBassBoost != NULL "
                "-> Calling pContext->pBassBoost->free()");

        LvmEffect_free(pContext);

        printf("\tLvmBundle_init pContext->pBassBoost != NULL "
                "-> Called pContext->pBassBoost->free()");
    }

    LVM_ReturnStatus_en     LvmStatus=LVM_SUCCESS;          /* Function call status */
    LVM_ControlParams_t     params;                         /* Control Parameters */
    LVM_InstParams_t        InstParams;                     /* Instance parameters */
    LVM_EQNB_BandDef_t      BandDefs[MAX_NUM_BANDS];        /* Equaliser band definitions */
    LVM_HeadroomParams_t    HeadroomParams;                 /* Headroom parameters */
    LVM_HeadroomBandDef_t   HeadroomBandDef[LVM_HEADROOM_MAX_NBANDS];
    LVM_MemTab_t            MemTab;                         /* Memory allocation table */
    bool                    bMallocFailure = LVM_FALSE;

    /* Set the capabilities */
    InstParams.BufferMode       = LVM_UNMANAGED_BUFFERS;
    InstParams.MaxBlockSize     = MAX_CALL_SIZE;
    InstParams.EQNB_NumBands    = MAX_NUM_BANDS;
    InstParams.PSA_Included     = LVM_PSA_ON;

    /* Allocate memory, forcing alignment */
    LvmStatus = LVM_GetMemoryTable(LVM_NULL,
                                  &MemTab,
                                  &InstParams);

    LVM_ERROR_CHECK(LvmStatus, "LVM_GetMemoryTable", "LvmBundle_init")
    if(LvmStatus != LVM_SUCCESS) return -EINVAL;

    printf("\tCreateInstance Succesfully called LVM_GetMemoryTable\n");

    /* Allocate memory */
    for (i=0; i<LVM_NR_MEMORY_REGIONS; i++){
        if (MemTab.Region[i].Size != 0){
            MemTab.Region[i].pBaseAddress = malloc(MemTab.Region[i].Size);

            if (MemTab.Region[i].pBaseAddress == LVM_NULL){
                printf("\tLVM_ERROR :LvmBundle_init CreateInstance Failed to allocate %ld bytes "
                        "for region %u\n", MemTab.Region[i].Size, i );
                bMallocFailure = LVM_TRUE;
            }else{
                printf("\tLvmBundle_init CreateInstance allocated %ld bytes for region %u at %p\n",
                        MemTab.Region[i].Size, i, MemTab.Region[i].pBaseAddress);
            }
        }
    }

    /* If one or more of the memory regions failed to allocate, free the regions that were
     * succesfully allocated and return with an error
     */
    if(bMallocFailure == LVM_TRUE){
        for (i=0; i<LVM_NR_MEMORY_REGIONS; i++){
            if (MemTab.Region[i].pBaseAddress == LVM_NULL){
                printf("\tLVM_ERROR :LvmBundle_init CreateInstance Failed to allocate %ld bytes "
                        "for region %u Not freeing\n", MemTab.Region[i].Size, i );
            }else{
                printf("\tLVM_ERROR :LvmBundle_init CreateInstance Failed: but allocated %ld bytes "
                     "for region %u at %p- free\n",
                     MemTab.Region[i].Size, i, MemTab.Region[i].pBaseAddress);
                free(MemTab.Region[i].pBaseAddress);
            }
        }
        return -EINVAL;
    }
    printf("\tLvmBundle_init CreateInstance Succesfully malloc'd memory\n");

    /* Initialise */
    pContext->pBundledContext->hInstance = LVM_NULL;
    
    /* Init sets the instance handle */
    LvmStatus = LVM_GetInstanceHandle(&pContext->pBundledContext->hInstance,
                                      &MemTab,
                                      &InstParams);

    LVM_ERROR_CHECK(LvmStatus, "LVM_GetInstanceHandle", "LvmBundle_init")
    if(LvmStatus != LVM_SUCCESS) return -EINVAL;

    printf("\tLvmBundle_init CreateInstance Succesfully called LVM_GetInstanceHandle\n");

    /* Set the initial process parameters */
    /* General parameters */
    params.OperatingMode          = LVM_MODE_ON;
    params.SampleRate             = LVM_FS_44100;
    params.SourceFormat           = LVM_STEREO;
    params.SpeakerType            = LVM_HEADPHONES;

    pContext->pBundledContext->SampleRate = LVM_FS_44100;

    /* Concert Sound parameters */
    params.VirtualizerOperatingMode   = LVM_MODE_OFF;
    params.VirtualizerType            = LVM_CONCERTSOUND;
    params.VirtualizerReverbLevel     = 100;
    params.CS_EffectLevel             = LVM_CS_EFFECT_NONE;

    /* N-Band Equaliser parameters */
    params.EQNB_OperatingMode     = LVM_EQNB_OFF;
    params.EQNB_NBands            = FIVEBAND_NUMBANDS;
    params.pEQNB_BandDefinition   = &BandDefs[0];

    for (i=0; i<FIVEBAND_NUMBANDS; i++)
    {
        BandDefs[i].Frequency = EQNB_5BandPresetsFrequencies[i];
        BandDefs[i].QFactor   = EQNB_5BandPresetsQFactors[i];
        BandDefs[i].Gain      = EQNB_5BandSoftPresets[i];
    }

    /* Volume Control parameters */
    params.VC_EffectLevel         = 0;
    params.VC_Balance             = 0;

    /* Treble Enhancement parameters */
    params.TE_OperatingMode       = LVM_TE_OFF;
    params.TE_EffectLevel         = 0;

    /* PSA Control parameters */
    params.PSA_Enable             = LVM_PSA_OFF;
    params.PSA_PeakDecayRate      = (LVM_PSA_DecaySpeed_en)0;

    /* Bass Enhancement parameters */
    params.BE_OperatingMode       = LVM_BE_OFF;
    params.BE_EffectLevel         = 0;
    params.BE_CentreFreq          = LVM_BE_CENTRE_90Hz;
    params.BE_HPF                 = LVM_BE_HPF_ON;

    /* PSA Control parameters */
    params.PSA_Enable             = LVM_PSA_OFF;
    params.PSA_PeakDecayRate      = LVM_PSA_SPEED_MEDIUM;

    /* TE Control parameters */
    params.TE_OperatingMode       = LVM_TE_OFF;
    params.TE_EffectLevel         = 0;

    /* Activate the initial settings */
    LvmStatus = LVM_SetControlParameters(pContext->pBundledContext->hInstance,
                                         &params);

    LVM_ERROR_CHECK(LvmStatus, "LVM_SetControlParameters", "LvmBundle_init")
    if(LvmStatus != LVM_SUCCESS) return -EINVAL;

    printf("\tLvmBundle_init CreateInstance Succesfully called LVM_SetControlParameters\n");

    /* Set the headroom parameters */
    HeadroomBandDef[0].Limit_Low          = 20;
    HeadroomBandDef[0].Limit_High         = 4999;
    HeadroomBandDef[0].Headroom_Offset    = 0;
    HeadroomBandDef[1].Limit_Low          = 5000;
    HeadroomBandDef[1].Limit_High         = 24000;
    HeadroomBandDef[1].Headroom_Offset    = 0;
    HeadroomParams.pHeadroomDefinition    = &HeadroomBandDef[0];
    HeadroomParams.Headroom_OperatingMode = LVM_HEADROOM_ON;
    HeadroomParams.NHeadroomBands         = 2;

    LvmStatus = LVM_SetHeadroomParams(pContext->pBundledContext->hInstance,
                                      &HeadroomParams);

    LVM_ERROR_CHECK(LvmStatus, "LVM_SetHeadroomParams", "LvmBundle_init")
    if(LvmStatus != LVM_SUCCESS) return -EINVAL;

    printf("\tLvmBundle_init CreateInstance Succesfully called LVM_SetHeadroomParams\n");
    printf("\tLvmBundle_init End");
    return 0;
}   /* end LvmBundle_init */


//----------------------------------------------------------------------------
// LvmBundle_process()
//----------------------------------------------------------------------------
// Purpose:
// Apply LVM Bundle effects
//
// Inputs:
//  pIn:        pointer to stereo 16 bit input data
//  pOut:       pointer to stereo 16 bit output data
//  frameCount: Frames to process
//  pContext:   effect engine context
//  strength    strength to be applied
//
//  Outputs:
//  pOut:       pointer to updated stereo 16 bit output data
//
//----------------------------------------------------------------------------

int LvmBundle_process(LVM_INT16        *pIn,
                      LVM_INT16        *pOut,
                      int              frameCount,
                      EffectContext    *pContext){

    //LVM_ControlParams_t     ActiveParams;                           /* Current control Parameters */
    LVM_ReturnStatus_en     LvmStatus = LVM_SUCCESS;                /* Function call status */
    LVM_INT16               *pOutTmp;
#if 0
    if (pContext->config.outputCfg.accessMode == EFFECT_BUFFER_ACCESS_WRITE){
        pOutTmp = pOut;
    }else if (pContext->config.outputCfg.accessMode == EFFECT_BUFFER_ACCESS_ACCUMULATE)
#endif
    {
        if (pContext->pBundledContext->frameCount != frameCount) {
            if (pContext->pBundledContext->workBuffer != NULL) {
                free(pContext->pBundledContext->workBuffer);
            }
            pContext->pBundledContext->workBuffer =
                    (LVM_INT16 *)malloc(frameCount * sizeof(LVM_INT16) * 2);
            pContext->pBundledContext->frameCount = frameCount;
        }
        pOutTmp = pContext->pBundledContext->workBuffer;
    }
#if 0
    else{
        printf("LVM_ERROR : LvmBundle_process invalid access mode");
        return -1;
    }
#endif
    //printf("Calling LVM_Process");

    /* Process the samples */
    LvmStatus = LVM_Process(pContext->pBundledContext->hInstance, /* Instance handle */
                            pIn,                                  /* Input buffer */
                            pOutTmp,                              /* Output buffer */
                            (LVM_UINT16)frameCount,               /* Number of samples to read */
                            0);                                   /* Audo Time */

    LVM_ERROR_CHECK(LvmStatus, "LVM_Process", "LvmBundle_process")
    if(LvmStatus != LVM_SUCCESS) return -EINVAL;

    int i;
    //if (pContext->config.outputCfg.accessMode == EFFECT_BUFFER_ACCESS_ACCUMULATE)
    {
        for (i=0; i<frameCount*2; i++){
            pOut[i] = clamp16((LVM_INT32)pOut[i] + (LVM_INT32)pOutTmp[i]);
        }
    }
    return 0;
}    /* end LvmBundle_process */

//----------------------------------------------------------------------------
// LvmEffect_enable()
//----------------------------------------------------------------------------
// Purpose: Enable the effect in the bundle
//
// Inputs:
//  pContext:   effect engine context
//
// Outputs:
//
//----------------------------------------------------------------------------

int LvmEffect_enable(EffectContext *pContext){
    //printf("\tLvmEffect_enable start");

    LVM_ControlParams_t     ActiveParams;                           /* Current control Parameters */
    LVM_ReturnStatus_en     LvmStatus = LVM_SUCCESS;                /* Function call status */

    /* Get the current settings */
    LvmStatus = LVM_GetControlParameters(pContext->pBundledContext->hInstance,
                                         &ActiveParams);

    LVM_ERROR_CHECK(LvmStatus, "LVM_GetControlParameters", "LvmEffect_enable")
    if(LvmStatus != LVM_SUCCESS) return -EINVAL;
    //printf("\tLvmEffect_enable Succesfully called LVM_GetControlParameters\n");

    if(pContext->EffectType == LVM_BASS_BOOST) {
        printf("\tLvmEffect_enable : Enabling LVM_BASS_BOOST");
        ActiveParams.BE_OperatingMode       = LVM_BE_ON;
    }
    if(pContext->EffectType == LVM_VIRTUALIZER) {
        printf("\tLvmEffect_enable : Enabling LVM_VIRTUALIZER");
        ActiveParams.VirtualizerOperatingMode   = LVM_MODE_ON;
    }
    if(pContext->EffectType == LVM_EQUALIZER) {
        printf("\tLvmEffect_enable : Enabling LVM_EQUALIZER");
        ActiveParams.EQNB_OperatingMode     = LVM_EQNB_ON;
    }
    if(pContext->EffectType == LVM_VOLUME) {
        printf("\tLvmEffect_enable : Enabling LVM_VOLUME");
    }

    LvmStatus = LVM_SetControlParameters(pContext->pBundledContext->hInstance, &ActiveParams);
    LVM_ERROR_CHECK(LvmStatus, "LVM_SetControlParameters", "LvmEffect_enable")
    if(LvmStatus != LVM_SUCCESS) return -EINVAL;

    //printf("\tLvmEffect_enable Succesfully called LVM_SetControlParameters\n");
    //printf("\tLvmEffect_enable end");
    return 0;
}

//----------------------------------------------------------------------------
// LvmEffect_disable()
//----------------------------------------------------------------------------
// Purpose: Disable the effect in the bundle
//
// Inputs:
//  pContext:   effect engine context
//
// Outputs:
//
//----------------------------------------------------------------------------

int LvmEffect_disable(EffectContext *pContext){
    //printf("\tLvmEffect_disable start");

    LVM_ControlParams_t     ActiveParams;                           /* Current control Parameters */
    LVM_ReturnStatus_en     LvmStatus = LVM_SUCCESS;                /* Function call status */
    /* Get the current settings */
    LvmStatus = LVM_GetControlParameters(pContext->pBundledContext->hInstance,
                                         &ActiveParams);

    LVM_ERROR_CHECK(LvmStatus, "LVM_GetControlParameters", "LvmEffect_disable")
    if(LvmStatus != LVM_SUCCESS) return -EINVAL;
    //printf("\tLvmEffect_disable Succesfully called LVM_GetControlParameters\n");

    if(pContext->EffectType == LVM_BASS_BOOST) {
        printf("\tLvmEffect_disable : Disabling LVM_BASS_BOOST");
        ActiveParams.BE_OperatingMode       = LVM_BE_OFF;
    }
    if(pContext->EffectType == LVM_VIRTUALIZER) {
        printf("\tLvmEffect_disable : Disabling LVM_VIRTUALIZER");
        ActiveParams.VirtualizerOperatingMode   = LVM_MODE_OFF;
    }
    if(pContext->EffectType == LVM_EQUALIZER) {
        printf("\tLvmEffect_disable : Disabling LVM_EQUALIZER");
        ActiveParams.EQNB_OperatingMode     = LVM_EQNB_OFF;
    }
    if(pContext->EffectType == LVM_VOLUME) {
        printf("\tLvmEffect_disable : Disabling LVM_VOLUME");
    }

    LvmStatus = LVM_SetControlParameters(pContext->pBundledContext->hInstance, &ActiveParams);
    LVM_ERROR_CHECK(LvmStatus, "LVM_SetControlParameters", "LvmEffect_disable")
    if(LvmStatus != LVM_SUCCESS) return -EINVAL;

    //printf("\tLvmEffect_disable Succesfully called LVM_SetControlParameters\n");
    //printf("\tLvmEffect_disable end");
    return 0;
}

//----------------------------------------------------------------------------
// LvmEffect_free()
//----------------------------------------------------------------------------
// Purpose: Free all memory associated with the Bundle.
//
// Inputs:
//  pContext:   effect engine context
//
// Outputs:
//
//----------------------------------------------------------------------------

void LvmEffect_free(EffectContext *pContext){
    LVM_ReturnStatus_en     LvmStatus=LVM_SUCCESS;         /* Function call status */
    //LVM_ControlParams_t     params;                        /* Control Parameters */
    LVM_MemTab_t            MemTab;
    int i;
    /* Free the algorithm memory */
    LvmStatus = LVM_GetMemoryTable(pContext->pBundledContext->hInstance,
                                   &MemTab,
                                   LVM_NULL);

    LVM_ERROR_CHECK(LvmStatus, "LVM_GetMemoryTable", "LvmEffect_free")

    for (i=0; i<LVM_NR_MEMORY_REGIONS; i++){
        if (MemTab.Region[i].Size != 0){
            if (MemTab.Region[i].pBaseAddress != NULL){
                printf("\tLvmEffect_free - START freeing %ld bytes for region %u at %p\n",
                        MemTab.Region[i].Size, i, MemTab.Region[i].pBaseAddress);

                free(MemTab.Region[i].pBaseAddress);

                printf("\tLvmEffect_free - END   freeing %ld bytes for region %u at %p\n",
                        MemTab.Region[i].Size, i, MemTab.Region[i].pBaseAddress);
            }else{
                printf("\tLVM_ERROR : LvmEffect_free - trying to free with NULL pointer %ld bytes "
                        "for region %u at %p ERROR\n",
                        MemTab.Region[i].Size, i, MemTab.Region[i].pBaseAddress);
            }
        }
    }
}    /* end LvmEffect_free */


static int bundle_init(dtap_context_t *ctx)
{
    int ret = 0;
    int i = 0;
    EffectContext *pContext = NULL;
    dtap_para_t *ppara = &ctx->para;    
    int sessionNo = 0;
    int sessionId = 0;

    pContext = (EffectContext *)malloc(sizeof(EffectContext)); 
    // If this is the first create in this session
    if(pContext)
    {
        pContext->pBundledContext = (BundledEffectContext *)malloc(sizeof(BundledEffectContext));
        pContext->pBundledContext->SessionNo                = sessionNo;
        pContext->pBundledContext->SessionId                = sessionId;
        pContext->pBundledContext->hInstance                = NULL;
        pContext->pBundledContext->bVolumeEnabled           = LVM_FALSE;
        pContext->pBundledContext->bEqualizerEnabled        = LVM_FALSE;
        pContext->pBundledContext->bBassEnabled             = LVM_FALSE;
        pContext->pBundledContext->bBassTempDisabled        = LVM_FALSE;
        pContext->pBundledContext->bVirtualizerEnabled      = LVM_FALSE;
        pContext->pBundledContext->bVirtualizerTempDisabled = LVM_FALSE;
        pContext->pBundledContext->NumberEffectsEnabled     = 0;
        pContext->pBundledContext->NumberEffectsCalled      = 0;
        pContext->pBundledContext->firstVolume              = LVM_TRUE;
        pContext->pBundledContext->volume                   = 0;

        /* Saved strength is used to return the exact strength that was used in the set to the get
         * because we map the original strength range of 0:1000 to 1:15, and this will avoid
         * quantisation like effect when returning
         */
        pContext->pBundledContext->BassStrengthSaved        = 0;
        pContext->pBundledContext->VirtStrengthSaved        = 0;
        pContext->pBundledContext->CurPreset                = PRESET_CUSTOM;
        pContext->pBundledContext->levelSaved               = 0;
        pContext->pBundledContext->bMuteEnabled             = LVM_FALSE;
        pContext->pBundledContext->bStereoPositionEnabled   = LVM_FALSE;
        pContext->pBundledContext->positionSaved            = 0;
        pContext->pBundledContext->workBuffer               = NULL;
        pContext->pBundledContext->frameCount               = -1;
        pContext->pBundledContext->SamplesToExitCountVirt   = 0;
        pContext->pBundledContext->SamplesToExitCountBb     = 0;
        pContext->pBundledContext->SamplesToExitCountEq     = 0;

        for (i = 0; i < FIVEBAND_NUMBANDS; i++) {
            pContext->pBundledContext->bandGaindB[i] = EQNB_5BandSoftPresets[i];
        }

        printf("\tEffectCreate - Calling LvmBundle_init \n");
        ret = LvmBundle_init(pContext);

        if (ret < 0){
            printf("\tLVM_ERROR : EffectCreate() Bundle init failed \n");
            goto exit;
        }

        //set config

        LVM_Fs_en   SampleRate;
        switch (ppara->samplerate) {
        case 8000:
            SampleRate = LVM_FS_8000;
            pContext->pBundledContext->SamplesPerSecond = 8000*2; // 2 secs Stereo
            break;
        case 16000:
            SampleRate = LVM_FS_16000;
            pContext->pBundledContext->SamplesPerSecond = 16000*2; // 2 secs Stereo
            break;
        case 22050:
            SampleRate = LVM_FS_22050;
            pContext->pBundledContext->SamplesPerSecond = 22050*2; // 2 secs Stereo
            break;
        case 32000:
            SampleRate = LVM_FS_32000;
            pContext->pBundledContext->SamplesPerSecond = 32000*2; // 2 secs Stereo
            break;
        case 44100:
            SampleRate = LVM_FS_44100;
            pContext->pBundledContext->SamplesPerSecond = 44100*2; // 2 secs Stereo
            break;
        case 48000:
            SampleRate = LVM_FS_48000;
            pContext->pBundledContext->SamplesPerSecond = 48000*2; // 2 secs Stereo
            break;
        default:
            printf("\tEffect_setConfig invalid sampling rate %d", ppara->samplerate);
            return -EINVAL;
        }

        if(pContext->pBundledContext->SampleRate != SampleRate){

            LVM_ControlParams_t     ActiveParams;
            LVM_ReturnStatus_en     LvmStatus = LVM_SUCCESS;

            printf("\tEffect_setConfig change sampling rate to %d", SampleRate);

            /* Get the current settings */
            LvmStatus = LVM_GetControlParameters(pContext->pBundledContext->hInstance,
                                         &ActiveParams);

            LVM_ERROR_CHECK(LvmStatus, "LVM_GetControlParameters", "Effect_setConfig")
            if(LvmStatus != LVM_SUCCESS) return -EINVAL;

            ActiveParams.SampleRate = SampleRate;

            LvmStatus = LVM_SetControlParameters(pContext->pBundledContext->hInstance, &ActiveParams);

            LVM_ERROR_CHECK(LvmStatus, "LVM_SetControlParameters", "Effect_setConfig")
            printf("\tEffect_setConfig Succesfully called LVM_SetControlParameters\n");
            pContext->pBundledContext->SampleRate = SampleRate;
        }
        
        //enable eq
        pContext->EffectType = LVM_EQUALIZER;
        LvmEffect_enable(pContext);

    }
    else
    {
        ret = -1;
        printf("\tEffectCreate - failed \n");
        goto exit;

    }

    ctx->ap_priv = (void *)pContext;
    printf("\tEffectCreate - pBundledContext is %p", pContext->pBundledContext);

exit:
    if (ret != 0) {
        if (pContext != NULL) {
            free(pContext);
        }
    }
    printf("\tEffectCreate end..\n\n");
    return ret;
}

static int bundle_process(dtap_context_t *ctx, dtap_frame_t *frame)
{
    EffectContext *pContext = (EffectContext *)ctx->ap_priv;
    dtap_para_t *ppara = &ctx->para;

    if(!ctx->out)
    {
        ctx->out = (uint8_t *)malloc(frame->in_size);
        ctx->out_size = frame->in_size;
    }

    if(ctx->out && ctx->out_size < frame->in_size)
    {
        free(ctx->out);
        ctx->out = (uint8_t *)malloc(frame->in_size);
        ctx->out_size = frame->in_size;
    }

    LVM_INT16 *pin = (LVM_INT16 *)frame->in;
    LVM_INT16 *pout = (LVM_INT16 *)ctx->out;
    int bytes_per_sample = ppara->channels * ppara->data_width / 8;
    int frameCount = frame->in_size / bytes_per_sample ; // 10ms once time
    //printf("\t framecount: %d insize:%d \n", frameCount, frame->in_size);
    LvmBundle_process(pin, pout, frameCount, pContext);

    memcpy(pin, pout, frame->in_size);
    //printf("\t process ok \n");

    return 0;
}

static int bundle_config(dtap_context_t *ctx)
{
    return 0;
}

static int bundle_release(dtap_context_t *ctx)
{
    EffectContext *pContext = (EffectContext *)ctx->ap_priv;
    if(ctx->out)
        free(ctx->out);
    ctx->out = NULL;
    ctx->out_size = 0;

    return 0;
}

ap_wrapper_t ap_android = {
    .id = DTAP_ID_ANDROID,
    .name = "andrpid audio effect",
    .init = bundle_init,
    .process = bundle_process,
    .config = bundle_config,
    .release = bundle_release
};

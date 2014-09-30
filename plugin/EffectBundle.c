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
#include "effect_equalizer.h"
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
#if 0
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
#endif

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
        
    pOutTmp = pOut;
#if 0
    if (pContext->config.outputCfg.accessMode == EFFECT_BUFFER_ACCESS_WRITE)
    {
        pOutTmp = pOut;
    }else if (pContext->config.outputCfg.accessMode == EFFECT_BUFFER_ACCESS_ACCUMULATE)
#endif
#if 0
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
#endif
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
#if 0
    int i;
    //if (pContext->config.outputCfg.accessMode == EFFECT_BUFFER_ACCESS_ACCUMULATE)
    {
        for (i=0; i<frameCount*2; i++){
            pOut[i] = clamp16((LVM_INT32)pOut[i] + (LVM_INT32)pOutTmp[i]);
        }
    }
#endif
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

    LVM_ERROR_CHECK(LvmStatus, "LVM_GetControlParameters", "LvmEffect_enable ")
    if(LvmStatus != LVM_SUCCESS) return -EINVAL;
    //printf("\tLvmEffect_enable Succesfully called LVM_GetControlParameters\n");

    if(pContext->EffectType == LVM_BASS_BOOST) {
        printf("\tLvmEffect_enable : Enabling LVM_BASS_BOOST \n");
        ActiveParams.BE_OperatingMode       = LVM_BE_ON;
    }
    if(pContext->EffectType == LVM_VIRTUALIZER) {
        printf("\tLvmEffect_enable : Enabling LVM_VIRTUALIZER \n");
        ActiveParams.VirtualizerOperatingMode   = LVM_MODE_ON;
    }
    if(pContext->EffectType == LVM_EQUALIZER) {
        printf("\tLvmEffect_enable : Enabling LVM_EQUALIZER \n");
        ActiveParams.EQNB_OperatingMode     = LVM_EQNB_ON;
    }
    if(pContext->EffectType == LVM_VOLUME) {
        printf("\tLvmEffect_enable : Enabling LVM_VOLUME \n");
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


//----------------------------------------------------------------------------
// EqualizerLimitBandLevels()
//----------------------------------------------------------------------------
// Purpose: limit all EQ band gains to a value less than 0 dB while
//          preserving the relative band levels.
//
// Inputs:
//  pContext:   effect engine context
//
// Outputs:
//
//----------------------------------------------------------------------------
void EqualizerLimitBandLevels(EffectContext *pContext) {
    LVM_ControlParams_t     ActiveParams;              /* Current control Parameters */
    LVM_ReturnStatus_en     LvmStatus=LVM_SUCCESS;     /* Function call status */

    /* Get the current settings */
    LvmStatus = LVM_GetControlParameters(pContext->pBundledContext->hInstance, &ActiveParams);
    LVM_ERROR_CHECK(LvmStatus, "LVM_GetControlParameters", "EqualizerLimitBandLevels")
    //printf("\tEqualizerLimitBandLevels Succesfully returned from LVM_GetControlParameters\n");
    //printf("\tEqualizerLimitBandLevels just Got -> %d\n",
    //          ActiveParams.pEQNB_BandDefinition[band].Gain);

    // Apply a volume correction to avoid clipping in the EQ based on 2 factors:
    // - the maximum EQ band gain: the volume correction is such that the total of volume + max
    // band gain is <= 0 dB
    // - the average gain in all bands weighted by their proximity to max gain band.
    int maxGain = 0;
    int avgGain = 0;
    int avgCount = 0;
    int i,j;
    for (i = 0; i < FIVEBAND_NUMBANDS; i++) {
        if (pContext->pBundledContext->bandGaindB[i] >= maxGain) {
            int tmpMaxGain = pContext->pBundledContext->bandGaindB[i];
            int tmpAvgGain = 0;
            int tmpAvgCount = 0;
            for (j = 0; j < FIVEBAND_NUMBANDS; j++) {
                int gain = pContext->pBundledContext->bandGaindB[j];
                // skip current band and gains < 0 dB
                if (j == i || gain < 0)
                    continue;
                // no need to continue if one band not processed yet has a higher gain than current
                // max
                if (gain > tmpMaxGain) {
                    // force skipping "if (tmpAvgGain >= avgGain)" below as tmpAvgGain is not
                    // meaningful in this case
                    tmpAvgGain = -1;
                    break;
                }

                int weight = 1;
                if (j < (i + 2) && j > (i - 2))
                    weight = 4;
                tmpAvgGain += weight * gain;
                tmpAvgCount += weight;
            }
            if (tmpAvgGain >= avgGain) {
                maxGain = tmpMaxGain;
                avgGain = tmpAvgGain;
                avgCount = tmpAvgCount;
            }
        }
        ActiveParams.pEQNB_BandDefinition[i].Frequency = EQNB_5BandPresetsFrequencies[i];
        ActiveParams.pEQNB_BandDefinition[i].QFactor   = EQNB_5BandPresetsQFactors[i];
        ActiveParams.pEQNB_BandDefinition[i].Gain = pContext->pBundledContext->bandGaindB[i];
    }

    int gainCorrection = 0;
    if (maxGain + pContext->pBundledContext->volume > 0) {
        gainCorrection = maxGain + pContext->pBundledContext->volume;
    }
    if (avgCount) {
        gainCorrection += avgGain/avgCount;
    }

    printf("EqualizerLimitBandLevels() gainCorrection %d maxGain %d avgGain %d avgCount %d \n",
            gainCorrection, maxGain, avgGain, avgCount);

    ActiveParams.VC_EffectLevel  = pContext->pBundledContext->volume - gainCorrection;
    if (ActiveParams.VC_EffectLevel < -96) {
        ActiveParams.VC_EffectLevel = -96;
    }

    /* Activate the initial settings */
    LvmStatus = LVM_SetControlParameters(pContext->pBundledContext->hInstance, &ActiveParams);
    LVM_ERROR_CHECK(LvmStatus, "LVM_SetControlParameters", "EqualizerLimitBandLevels")
    //printf("\tEqualizerLimitBandLevels just Set -> %d\n",
    //          ActiveParams.pEQNB_BandDefinition[band].Gain);

    //printf("\tEqualizerLimitBandLevels just set (-96dB -> 0dB)   -> %d\n",ActiveParams.VC_EffectLevel );
    if(pContext->pBundledContext->firstVolume == LVM_TRUE){
        LvmStatus = LVM_SetVolumeNoSmoothing(pContext->pBundledContext->hInstance, &ActiveParams);
        LVM_ERROR_CHECK(LvmStatus, "LVM_SetVolumeNoSmoothing", "LvmBundle_process")
        printf("\tLVM_VOLUME: Disabling Smoothing for first volume change to remove spikes/clicks \n");
        pContext->pBundledContext->firstVolume = LVM_FALSE;
    }
}


//----------------------------------------------------------------------------
// EqualizerGetBandLevel()
//----------------------------------------------------------------------------
// Purpose: Retrieve the gain currently being used for the band passed in
//
// Inputs:
//  band:       band number
//  pContext:   effect engine context
//
// Outputs:
//
//----------------------------------------------------------------------------
int32_t EqualizerGetBandLevel(EffectContext *pContext, int32_t band){
    //printf("\tEqualizerGetBandLevel -> %d\n", pContext->pBundledContext->bandGaindB[band] );
    return pContext->pBundledContext->bandGaindB[band] * 100;
}

//----------------------------------------------------------------------------
// EqualizerSetBandLevel()
//----------------------------------------------------------------------------
// Purpose:
//  Sets gain value for the given band.
//
// Inputs:
//  band:       band number
//  Gain:       Gain to be applied in millibels
//  pContext:   effect engine context
//
// Outputs:
//
//---------------------------------------------------------------------------
void EqualizerSetBandLevel(EffectContext *pContext, int band, short Gain){
    int gainRounded;
    if(Gain > 0){
        gainRounded = (int)((Gain+50)/100);
    }else{
        gainRounded = (int)((Gain-50)/100);
    }
    //printf("\tEqualizerSetBandLevel(%d)->(%d)", Gain, gainRounded);
    pContext->pBundledContext->bandGaindB[band] = gainRounded;
    pContext->pBundledContext->CurPreset = PRESET_CUSTOM;

    EqualizerLimitBandLevels(pContext);
}

//----------------------------------------------------------------------------
// EqualizerGetCentreFrequency()
//----------------------------------------------------------------------------
// Purpose: Retrieve the frequency being used for the band passed in
//
// Inputs:
//  band:       band number
//  pContext:   effect engine context
//
// Outputs:
//
//----------------------------------------------------------------------------
int32_t EqualizerGetCentreFrequency(EffectContext *pContext, int32_t band){
    int32_t Frequency =0;

    LVM_ControlParams_t     ActiveParams;                           /* Current control Parameters */
    LVM_ReturnStatus_en     LvmStatus = LVM_SUCCESS;                /* Function call status */
    LVM_EQNB_BandDef_t      *BandDef;
    /* Get the current settings */
    LvmStatus = LVM_GetControlParameters(pContext->pBundledContext->hInstance,
                                         &ActiveParams);

    LVM_ERROR_CHECK(LvmStatus, "LVM_GetControlParameters", "EqualizerGetCentreFrequency")

    BandDef   = ActiveParams.pEQNB_BandDefinition;
    Frequency = (int32_t)BandDef[band].Frequency*1000;     // Convert to millibels

    //printf("\tEqualizerGetCentreFrequency -> %d\n", Frequency );
    //printf("\tEqualizerGetCentreFrequency Succesfully returned from LVM_GetControlParameters\n");
    return Frequency;
}

//----------------------------------------------------------------------------
// EqualizerGetBandFreqRange(
//----------------------------------------------------------------------------
// Purpose:
//
// Gets lower and upper boundaries of a band.
// For the high shelf, the low bound is the band frequency and the high
// bound is Nyquist.
// For the peaking filters, they are the gain[dB]/2 points.
//
// Inputs:
//  band:       band number
//  pContext:   effect engine context
//
// Outputs:
//  pLow:       lower band range
//  pLow:       upper band range
//----------------------------------------------------------------------------
int32_t EqualizerGetBandFreqRange(EffectContext *pContext, int32_t band, uint32_t *pLow,
                                  uint32_t *pHi){
    *pLow = bandFreqRange[band][0];
    *pHi  = bandFreqRange[band][1];
    return 0;
}

//----------------------------------------------------------------------------
// EqualizerGetBand(
//----------------------------------------------------------------------------
// Purpose:
//
// Returns the band with the maximum influence on a given frequency.
// Result is unaffected by whether EQ is enabled or not, or by whether
// changes have been committed or not.
//
// Inputs:
//  targetFreq   The target frequency, in millihertz.
//  pContext:    effect engine context
//
// Outputs:
//  pLow:       lower band range
//  pLow:       upper band range
//----------------------------------------------------------------------------
int32_t EqualizerGetBand(EffectContext *pContext, uint32_t targetFreq){
    int band = 0;
    int i;

    if(targetFreq < bandFreqRange[0][0]){
        return -EINVAL;
    }else if(targetFreq == bandFreqRange[0][0]){
        return 0;
    }
    for(i=0; i<FIVEBAND_NUMBANDS;i++){
        if((targetFreq > bandFreqRange[i][0])&&(targetFreq <= bandFreqRange[i][1])){
            band = i;
        }
    }
    return band;
}

//----------------------------------------------------------------------------
// EqualizerGetPreset(
//----------------------------------------------------------------------------
// Purpose:
//
// Gets the currently set preset ID.
// Will return PRESET_CUSTOM in case the EQ parameters have been modified
// manually since a preset was set.
//
// Inputs:
//  pContext:    effect engine context
//
//----------------------------------------------------------------------------
int32_t EqualizerGetPreset(EffectContext *pContext){
    return pContext->pBundledContext->CurPreset;
}

//----------------------------------------------------------------------------
// EqualizerSetPreset(
//----------------------------------------------------------------------------
// Purpose:
//
// Sets the current preset by ID.
// All the band parameters will be overridden.
//
// Inputs:
//  pContext:    effect engine context
//  preset       The preset ID.
//
//----------------------------------------------------------------------------
void EqualizerSetPreset(EffectContext *pContext, int preset){
    int i;
    //printf("\tEqualizerSetPreset(%d)", preset);
    pContext->pBundledContext->CurPreset = preset;

    //ActiveParams.pEQNB_BandDefinition = &BandDefs[0];
    for (i=0; i<FIVEBAND_NUMBANDS; i++)
    {
        pContext->pBundledContext->bandGaindB[i] =
                EQNB_5BandSoftPresets[i + preset * FIVEBAND_NUMBANDS];
    }

    EqualizerLimitBandLevels(pContext);

    //printf("\tEqualizerSetPreset Succesfully called LVM_SetControlParameters\n");
    return;
}

int32_t EqualizerGetNumPresets(){
    return sizeof(gEqualizerPresets) / sizeof(PresetConfig);
}

//----------------------------------------------------------------------------
// EqualizerGetPresetName(
//----------------------------------------------------------------------------
// Purpose:
// Gets a human-readable name for a preset ID. Will return "Custom" if
// PRESET_CUSTOM is passed.
//
// Inputs:
// preset       The preset ID. Must be less than number of presets.
//
//-------------------------------------------------------------------------
const char * EqualizerGetPresetName(int32_t preset){
    //printf("\tEqualizerGetPresetName start(%d)", preset);
    if (preset == PRESET_CUSTOM) {
        return "Custom";
    } else {
        return gEqualizerPresets[preset].name;
    }
    //printf("\tEqualizerGetPresetName end(%d)", preset);
    return 0;
}

//----------------------------------------------------------------------------
// Equalizer_getParameter()
//----------------------------------------------------------------------------
// Purpose:
// Get a Equalizer parameter
//
// Inputs:
//  pEqualizer       - handle to instance data
//  pParam           - pointer to parameter
//  pValue           - pointer to variable to hold retrieved value
//  pValueSize       - pointer to value size: maximum size as input
//
// Outputs:
//  *pValue updated with parameter value
//  *pValueSize updated with actual value size
//
//
// Side Effects:
//
//----------------------------------------------------------------------------
int Equalizer_getParameter(EffectContext     *pContext,
                           void              *pParam,
                           size_t            *pValueSize,
                           void              *pValue){
    int status = 0;
    int32_t *pParamTemp = (int32_t *)pParam;
    int32_t param = *pParamTemp++;
    int32_t param2;
    char *name;
    int i;

    //printf("\tEqualizer_getParameter start");

    switch (param) {
    case EQ_PARAM_NUM_BANDS:
    case EQ_PARAM_CUR_PRESET:
    case EQ_PARAM_GET_NUM_OF_PRESETS:
    case EQ_PARAM_BAND_LEVEL:
    case EQ_PARAM_GET_BAND:
        if (*pValueSize < sizeof(int16_t)) {
            printf("LVM_ERROR : Equalizer_getParameter() invalid pValueSize 1  %zu", *pValueSize);
            return -EINVAL;
        }
        *pValueSize = sizeof(int16_t);
        break;

    case EQ_PARAM_LEVEL_RANGE:
        if (*pValueSize < 2 * sizeof(int16_t)) {
            printf("LVM_ERROR : Equalizer_getParameter() invalid pValueSize 2  %zu", *pValueSize);
            return -EINVAL;
        }
        *pValueSize = 2 * sizeof(int16_t);
        break;
    case EQ_PARAM_BAND_FREQ_RANGE:
        if (*pValueSize < 2 * sizeof(int32_t)) {
            printf("LVM_ERROR : Equalizer_getParameter() invalid pValueSize 3  %zu", *pValueSize);
            return -EINVAL;
        }
        *pValueSize = 2 * sizeof(int32_t);
        break;

    case EQ_PARAM_CENTER_FREQ:
        if (*pValueSize < sizeof(int32_t)) {
            printf("LVM_ERROR : Equalizer_getParameter() invalid pValueSize 5  %zu", *pValueSize);
            return -EINVAL;
        }
        *pValueSize = sizeof(int32_t);
        break;

    case EQ_PARAM_GET_PRESET_NAME:
        break;

    case EQ_PARAM_PROPERTIES:
        if (*pValueSize < (2 + FIVEBAND_NUMBANDS) * sizeof(uint16_t)) {
            printf("LVM_ERROR : Equalizer_getParameter() invalid pValueSize 1  %zu", *pValueSize);
            return -EINVAL;
        }
        *pValueSize = (2 + FIVEBAND_NUMBANDS) * sizeof(uint16_t);
        break;

    default:
        printf("LVM_ERROR : Equalizer_getParameter unknown param %d", param);
        return -EINVAL;
    }

    switch (param) {
    case EQ_PARAM_NUM_BANDS:
        *(uint16_t *)pValue = (uint16_t)FIVEBAND_NUMBANDS;
        //printf("\tEqualizer_getParameter() EQ_PARAM_NUM_BANDS %d", *(int16_t *)pValue);
        break;

    case EQ_PARAM_LEVEL_RANGE:
        *(int16_t *)pValue = -1500;
        *((int16_t *)pValue + 1) = 1500;
        //printf("\tEqualizer_getParameter() EQ_PARAM_LEVEL_RANGE min %d, max %d",
        //      *(int16_t *)pValue, *((int16_t *)pValue + 1));
        break;

    case EQ_PARAM_BAND_LEVEL:
        param2 = *pParamTemp;
        if (param2 >= FIVEBAND_NUMBANDS) {
            status = -EINVAL;
            break;
        }
        *(int16_t *)pValue = (int16_t)EqualizerGetBandLevel(pContext, param2);
        //printf("\tEqualizer_getParameter() EQ_PARAM_BAND_LEVEL band %d, level %d",
        //      param2, *(int32_t *)pValue);
        break;

    case EQ_PARAM_CENTER_FREQ:
        param2 = *pParamTemp;
        if (param2 >= FIVEBAND_NUMBANDS) {
            status = -EINVAL;
            break;
        }
        *(int32_t *)pValue = EqualizerGetCentreFrequency(pContext, param2);
        //printf("\tEqualizer_getParameter() EQ_PARAM_CENTER_FREQ band %d, frequency %d",
        //      param2, *(int32_t *)pValue);
        break;

    case EQ_PARAM_BAND_FREQ_RANGE:
        param2 = *pParamTemp;
        if (param2 >= FIVEBAND_NUMBANDS) {
            status = -EINVAL;
            break;
        }
        EqualizerGetBandFreqRange(pContext, param2, (uint32_t *)pValue, ((uint32_t *)pValue + 1));
        //printf("\tEqualizer_getParameter() EQ_PARAM_BAND_FREQ_RANGE band %d, min %d, max %d",
        //      param2, *(int32_t *)pValue, *((int32_t *)pValue + 1));
        break;

    case EQ_PARAM_GET_BAND:
        param2 = *pParamTemp;
        *(uint16_t *)pValue = (uint16_t)EqualizerGetBand(pContext, param2);
        //printf("\tEqualizer_getParameter() EQ_PARAM_GET_BAND frequency %d, band %d",
        //      param2, *(uint16_t *)pValue);
        break;

    case EQ_PARAM_CUR_PRESET:
        *(uint16_t *)pValue = (uint16_t)EqualizerGetPreset(pContext);
        //printf("\tEqualizer_getParameter() EQ_PARAM_CUR_PRESET %d", *(int32_t *)pValue);
        break;

    case EQ_PARAM_GET_NUM_OF_PRESETS:
        *(uint16_t *)pValue = (uint16_t)EqualizerGetNumPresets();
        //printf("\tEqualizer_getParameter() EQ_PARAM_GET_NUM_OF_PRESETS %d", *(int16_t *)pValue);
        break;

    case EQ_PARAM_GET_PRESET_NAME:
        param2 = *pParamTemp;
        if (param2 >= EqualizerGetNumPresets()) {
        //if (param2 >= 20) {     // AGO FIX
            status = -EINVAL;
            break;
        }
        name = (char *)pValue;
        strncpy(name, EqualizerGetPresetName(param2), *pValueSize - 1);
        name[*pValueSize - 1] = 0;
        *pValueSize = strlen(name) + 1;
        //printf("\tEqualizer_getParameter() EQ_PARAM_GET_PRESET_NAME preset %d, name %s len %d",
        //      param2, gEqualizerPresets[param2].name, *pValueSize);
        break;

    case EQ_PARAM_PROPERTIES: {
        int16_t *p = (int16_t *)pValue;
        printf("Equalizer_getParameter() EQ_PARAM_PROPERTIES");
        p[0] = (int16_t)EqualizerGetPreset(pContext);
        p[1] = (int16_t)FIVEBAND_NUMBANDS;
        for (i = 0; i < FIVEBAND_NUMBANDS; i++) {
            p[2 + i] = (int16_t)EqualizerGetBandLevel(pContext, i);
        }
    } break;

    default:
        printf("LVM_ERROR : Equalizer_getParameter() invalid param %d", param);
        status = -EINVAL;
        break;
    }

    //GV("\tEqualizer_getParameter end\n");
    return status;
} /* end Equalizer_getParameter */

//----------------------------------------------------------------------------
// Equalizer_setParameter()
//----------------------------------------------------------------------------
// Purpose:
// Set a Equalizer parameter
//
// Inputs:
//  pEqualizer    - handle to instance data
//  pParam        - pointer to parameter
//  pValue        - pointer to value
//
// Outputs:
//
//----------------------------------------------------------------------------
int Equalizer_setParameter (EffectContext *pContext, void *pParam, void *pValue){
    int status = 0;
    int32_t preset;
    int32_t band;
    int32_t level;
    int32_t *pParamTemp = (int32_t *)pParam;
    int32_t param = *pParamTemp++;


    //printf("\tEqualizer_setParameter start");
    switch (param) {
    case EQ_PARAM_CUR_PRESET:
        preset = (int32_t)(*(uint16_t *)pValue);

        //printf("\tEqualizer_setParameter() EQ_PARAM_CUR_PRESET %d", preset);
        if ((preset >= EqualizerGetNumPresets())||(preset < 0)) {
            status = -EINVAL;
            break;
        }
        EqualizerSetPreset(pContext, preset);
        break;
    case EQ_PARAM_BAND_LEVEL:
        band =  *pParamTemp;
        level = (int32_t)(*(int16_t *)pValue);
        //printf("\tEqualizer_setParameter() EQ_PARAM_BAND_LEVEL band %d, level %d", band, level);
        if (band >= FIVEBAND_NUMBANDS) {
            status = -EINVAL;
            break;
        }
        EqualizerSetBandLevel(pContext, band, level);
        break;
    case EQ_PARAM_PROPERTIES: {
        //printf("\tEqualizer_setParameter() EQ_PARAM_PROPERTIES");
        int16_t *p = (int16_t *)pValue;
        int i;
        if ((int)p[0] >= EqualizerGetNumPresets()) {
            status = -EINVAL;
            break;
        }
        if (p[0] >= 0) {
            EqualizerSetPreset(pContext, (int)p[0]);
        } else {
            if ((int)p[1] != FIVEBAND_NUMBANDS) {
                status = -EINVAL;
                break;
            }
            for (i = 0; i < FIVEBAND_NUMBANDS; i++) {
                EqualizerSetBandLevel(pContext, i, (int)p[2 + i]);
            }
        }
    } break;
    default:
        printf("\tLVM_ERROR : Equalizer_setParameter() invalid param %d", param);
        status = -EINVAL;
        break;
    }

    //printf("\tEqualizer_setParameter end");
    return status;
} /* end Equalizer_setParameter */

//----------------------------------------------------------------------------
// Effect_setEnabled()
//----------------------------------------------------------------------------
// Purpose:
// Enable or disable effect
//
// Inputs:
//  pContext      - pointer to effect context
//  enabled       - true if enabling the effect, false otherwise
//
// Outputs:
//
//----------------------------------------------------------------------------

int Effect_setEnabled(EffectContext *pContext, bool enabled)
{
    printf("\tEffect_setEnabled() type %d, enabled %d", pContext->EffectType, enabled);

    if (enabled) {
        // Bass boost or Virtualizer can be temporarily disabled if playing over device speaker due
        // to their nature.
        bool tempDisabled = false;
        switch (pContext->EffectType) {
            case LVM_BASS_BOOST:
                if (pContext->pBundledContext->bBassEnabled == LVM_TRUE) {
                     printf("\tEffect_setEnabled() LVM_BASS_BOOST is already enabled \n");
                     return -EINVAL;
                }
                if(pContext->pBundledContext->SamplesToExitCountBb <= 0){
                    pContext->pBundledContext->EffectsBitMap |= (1 << LVM_BASS_BOOST);
                }
                pContext->pBundledContext->SamplesToExitCountBb =
                     (LVM_INT32)(pContext->pBundledContext->SamplesPerSecond*0.1);
                pContext->pBundledContext->bBassEnabled = LVM_TRUE;
                tempDisabled = pContext->pBundledContext->bBassTempDisabled;
                break;
            case LVM_EQUALIZER:
                if (pContext->pBundledContext->bEqualizerEnabled == LVM_TRUE) {
                    printf("\tEffect_setEnabled() LVM_EQUALIZER is already enabled \n");
                    return -EINVAL;
                }
                if(pContext->pBundledContext->SamplesToExitCountEq <= 0){
                    pContext->pBundledContext->EffectsBitMap |= (1 << LVM_EQUALIZER);
                }
                pContext->pBundledContext->SamplesToExitCountEq =
                     (LVM_INT32)(pContext->pBundledContext->SamplesPerSecond*0.1);
                pContext->pBundledContext->bEqualizerEnabled = LVM_TRUE;
                break;
            case LVM_VIRTUALIZER:
                if (pContext->pBundledContext->bVirtualizerEnabled == LVM_TRUE) {
                    printf("\tEffect_setEnabled() LVM_VIRTUALIZER is already enabled \n");
                    return -EINVAL;
                }
                if(pContext->pBundledContext->SamplesToExitCountVirt <= 0){
                    pContext->pBundledContext->EffectsBitMap |= (1 << LVM_VIRTUALIZER);
                }
                pContext->pBundledContext->SamplesToExitCountVirt =
                     (LVM_INT32)(pContext->pBundledContext->SamplesPerSecond*0.1);
                pContext->pBundledContext->bVirtualizerEnabled = LVM_TRUE;
                tempDisabled = pContext->pBundledContext->bVirtualizerTempDisabled;
                break;
            case LVM_VOLUME:
                if (pContext->pBundledContext->bVolumeEnabled == LVM_TRUE) {
                    printf("\tEffect_setEnabled() LVM_VOLUME is already enabled \n");
                    return -EINVAL;
                }
                pContext->pBundledContext->EffectsBitMap |= (1 << LVM_VOLUME);
                pContext->pBundledContext->bVolumeEnabled = LVM_TRUE;
                break;
            default:
                printf("\tEffect_setEnabled() invalid effect type \n");
                return -EINVAL;
        }
        if (!tempDisabled) {
            LvmEffect_enable(pContext);
        }
    } else {
        switch (pContext->EffectType) {
            case LVM_BASS_BOOST:
                if (pContext->pBundledContext->bBassEnabled == LVM_FALSE) {
                    printf("\tEffect_setEnabled() LVM_BASS_BOOST is already disabled \n");
                    return -EINVAL;
                }
                if(pContext->pBundledContext->SamplesToExitCountBb <= 0) {
                    pContext->pBundledContext->EffectsBitMap &= ~(1 << LVM_BASS_BOOST);
                }
                pContext->pBundledContext->bBassEnabled = LVM_FALSE;
                break;
            case LVM_EQUALIZER:
                if (pContext->pBundledContext->bEqualizerEnabled == LVM_FALSE) {
                    printf("\tEffect_setEnabled() LVM_EQUALIZER is already disabled \n");
                    return -EINVAL;
                }
                if(pContext->pBundledContext->SamplesToExitCountEq <= 0) {
                    pContext->pBundledContext->EffectsBitMap &= ~(1 << LVM_EQUALIZER);
                }
                pContext->pBundledContext->bEqualizerEnabled = LVM_FALSE;
                break;
            case LVM_VIRTUALIZER:
                if (pContext->pBundledContext->bVirtualizerEnabled == LVM_FALSE) {
                    printf("\tEffect_setEnabled() LVM_VIRTUALIZER is already disabled \n");
                    return -EINVAL;
                }
                if(pContext->pBundledContext->SamplesToExitCountVirt <= 0) {
                    pContext->pBundledContext->EffectsBitMap &= ~(1 << LVM_VIRTUALIZER);
                }
                pContext->pBundledContext->bVirtualizerEnabled = LVM_FALSE;
                break;
            case LVM_VOLUME:
                if (pContext->pBundledContext->bVolumeEnabled == LVM_FALSE) {
                    printf("\tEffect_setEnabled() LVM_VOLUME is already disabled \n");
                    return -EINVAL;
                }
                pContext->pBundledContext->EffectsBitMap &= ~(1 << LVM_VOLUME);
                pContext->pBundledContext->bVolumeEnabled = LVM_FALSE;
                break;
            default:
                printf("\tEffect_setEnabled() invalid effect type \n");
                return -EINVAL;
        }
        LvmEffect_disable(pContext);
    }

    return 0;
}



static int lvm_eq_init(dtap_context_t *ctx)
{
    int ret = 0;
    int i = 0;
    EffectContext *pContext = NULL;
    dtap_para_t *ppara = &ctx->para;    
    int sessionNo = 0;
    int sessionId = 0;

    pContext = (EffectContext *)malloc(sizeof(EffectContext)); 
    if(pContext == NULL)
    {
        ret = -1;
        goto exit;
    }
    // If this is the first create in this session
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

    //printf("\tEffectCreate - Calling LvmBundle_init \n");
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

        //printf("\tEffect_setConfig change sampling rate to %d", SampleRate);

        /* Get the current settings */
        LvmStatus = LVM_GetControlParameters(pContext->pBundledContext->hInstance,
                                         &ActiveParams);

        LVM_ERROR_CHECK(LvmStatus, "LVM_GetControlParameters", "Effect_setConfig")
        if(LvmStatus != LVM_SUCCESS) return -EINVAL;

        ActiveParams.SampleRate = SampleRate;

        LvmStatus = LVM_SetControlParameters(pContext->pBundledContext->hInstance, &ActiveParams);

        LVM_ERROR_CHECK(LvmStatus, "LVM_SetControlParameters", "Effect_setConfig")
        //printf("Effect_setConfig Succesfully called LVM_SetControlParameters\n");
        pContext->pBundledContext->SampleRate = SampleRate;
    }
   
    if(ppara->type == DTAP_EFFECT_EQ)
    {
        printf("eq enable, item:%d  \n", ppara->item);
        //enable eq
        pContext->EffectType = LVM_EQUALIZER;
        Effect_setEnabled(pContext, 1);
        EqualizerSetPreset(pContext, ppara->item);
    }
    else
    {
        printf("eq support only \n");
        ret = -1;
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

static int lvm_eq_process(dtap_context_t *ctx, dtap_frame_t *frame)
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

    memset(ctx->out, 0, ctx->out_size);

    int bytes_per_sample = ppara->channels * ppara->data_width / 8;
    int frameCount = frame->in_size / bytes_per_sample ; // 10ms once time
    //printf("\t framecount: %d insize:%d \n", frameCount, frame->in_size);
    LvmBundle_process(pin, pout, frameCount, pContext);

    memcpy(pin, pout, frame->in_size);
    //printf("\t process ok \n");

    return 0;
}

static int lvm_eq_release(dtap_context_t *ctx)
{
    //EffectContext *pContext = (EffectContext *)ctx->ap_priv;
    if(ctx->out)
        free(ctx->out);
    ctx->out = NULL;
    ctx->out_size = 0;

    return 0;
}

ap_wrapper_t ap_lvm_eq = {
    .id = DTAP_ID_LVM,
    .name = "andrpid audio effect",
    .init = lvm_eq_init,
    .process = lvm_eq_process,
    .release = lvm_eq_release
};

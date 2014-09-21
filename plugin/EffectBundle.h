/*
 * Copyright (C) 2010 The Android Open Source Project
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

#ifndef ANDROID_EFFECTBUNDLE_H_
#define ANDROID_EFFECTBUNDLE_H_

#include <LVM.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>

#if __cplusplus
extern "C" {
#endif

#define FIVEBAND_NUMBANDS          5
#define MAX_NUM_BANDS              5
#define MAX_CALL_SIZE              256
#define LVM_MAX_SESSIONS           32
#define LVM_UNUSED_SESSION         INT_MAX
#define BASS_BOOST_CUP_LOAD_ARM9E  150    // Expressed in 0.1 MIPS
#define VIRTUALIZER_CUP_LOAD_ARM9E 120    // Expressed in 0.1 MIPS
#define EQUALIZER_CUP_LOAD_ARM9E   220    // Expressed in 0.1 MIPS
#define VOLUME_CUP_LOAD_ARM9E      0      // Expressed in 0.1 MIPS
#define BUNDLE_MEM_USAGE           25     // Expressed in kB
//#define LVM_PCM

typedef enum
{
    LVM_BASS_BOOST,
    LVM_VIRTUALIZER,
    LVM_EQUALIZER,
    LVM_VOLUME
} lvm_effect_en;

// Preset configuration.
typedef struct PresetConfig {
    // Human-readable name.
    const char * name;
    // An array of size nBands where each element is a configuration for the
    // corresponding band.
    //const BandConfig * bandConfigs;
}PresetConfig;

/* BundledEffectContext : One per session */
typedef struct BundledEffectContext{
    LVM_Handle_t                    hInstance;                /* Instance handle */
    int                             SessionNo;                /* Current session number */
    int                             SessionId;                /* Current session id */
    bool                            bVolumeEnabled;           /* Flag for Volume */
    bool                            bEqualizerEnabled;        /* Flag for EQ */
    bool                            bBassEnabled;             /* Flag for Bass */
    bool                            bBassTempDisabled;        /* Flag for Bass to be re-enabled */
    bool                            bVirtualizerEnabled;      /* Flag for Virtualizer */
    bool                            bVirtualizerTempDisabled; /* Flag for effect to be re-enabled */
    int                             NumberEffectsEnabled;     /* Effects in this session */
    int                             NumberEffectsCalled;      /* Effects called so far */
    bool                            firstVolume;              /* No smoothing on first Vol change */
    // Saved parameters for each effect */
    // Bass Boost
    int                             BassStrengthSaved;        /* Conversion between Get/Set */
    // Equalizer
    int                             CurPreset;                /* Current preset being used */
    // Virtualzer
    int                             VirtStrengthSaved;        /* Conversion between Get/Set */
    // Volume
    int                             levelSaved;     /* for when mute is set, level must be saved */
    int                             positionSaved;
    bool                            bMuteEnabled;   /* Must store as mute = -96dB level */
    bool                            bStereoPositionEnabled;
    LVM_Fs_en                       SampleRate;
    int                             SamplesPerSecond;
    int                             SamplesToExitCountEq;
    int                             SamplesToExitCountBb;
    int                             SamplesToExitCountVirt;
    LVM_INT16                       *workBuffer;
    int                             frameCount;
    int32_t                         bandGaindB[FIVEBAND_NUMBANDS];
    int                             volume;
    #ifdef LVM_PCM
    FILE                            *PcmInPtr;
    FILE                            *PcmOutPtr;
    #endif
}BundledEffectContext;

/* SessionContext : One session */
typedef struct SessionContext{
    bool                            bBundledEffectsEnabled;
    bool                            bVolumeInstantiated;
    bool                            bEqualizerInstantiated;
    bool                            bBassInstantiated;
    bool                            bVirtualizerInstantiated;
    BundledEffectContext            *pBundledContext;
}SessionContext;

typedef struct EffectContext{
//    const struct effect_interface_s *itfe;
//    effect_config_t                 config;
    lvm_effect_en                   EffectType;
    BundledEffectContext            *pBundledContext;
}EffectContext;

/* enumerated parameter settings for Volume effect */
typedef enum
{
    VOLUME_PARAM_LEVEL,                       // type SLmillibel = typedef SLuint16 (set & get)
    VOLUME_PARAM_MAXLEVEL,                    // type SLmillibel = typedef SLuint16 (get)
    VOLUME_PARAM_MUTE,                        // type SLboolean  = typedef SLuint32 (set & get)
    VOLUME_PARAM_ENABLESTEREOPOSITION,        // type SLboolean  = typedef SLuint32 (set & get)
    VOLUME_PARAM_STEREOPOSITION,              // type SLpermille = typedef SLuint16 (set & get)
} t_volume_params;

static const int PRESET_CUSTOM = -1;

static const uint32_t bandFreqRange[FIVEBAND_NUMBANDS][2] = {
                                       {30000, 120000},
                                       {120001, 460000},
                                       {460001, 1800000},
                                       {1800001, 7000000},
                                       {7000001, 1}};

static const LVM_UINT16  EQNB_5BandPresetsFrequencies[] = {
                                       60,           /* Frequencies in Hz */
                                       230,
                                       910,
                                       3600,
                                       14000};

static const LVM_UINT16 EQNB_5BandPresetsQFactors[] = {
                                       96,               /* Q factor multiplied by 100 */
                                       96,
                                       96,
                                       96,
                                       96};

static const LVM_INT16 EQNB_5BandNormalPresets[] = {
                                       3, 0, 0, 0, 3,       /* Normal Preset */
                                       8, 5, -3, 5, 6,      /* Classical Preset */
                                       15, -6, 7, 13, 10,   /* Dance Preset */
                                       0, 0, 0, 0, 0,       /* Flat Preset */
                                       6, -2, -2, 6, -3,    /* Folk Preset */
                                       8, -8, 13, -1, -4,   /* Heavy Metal Preset */
                                       10, 6, -4, 5, 8,     /* Hip Hop Preset */
                                       8, 5, -4, 5, 9,      /* Jazz Preset */
                                      -6, 4, 9, 4, -5,      /* Pop Preset */
                                       10, 6, -1, 8, 10};   /* Rock Preset */

static const LVM_INT16 EQNB_5BandSoftPresets[] = {
                                        3, 0, 0, 0, 3,      /* Normal Preset */
                                        5, 3, -2, 4, 4,     /* Classical Preset */
                                        6, 0, 2, 4, 1,      /* Dance Preset */
                                        0, 0, 0, 0, 0,      /* Flat Preset */
                                        3, 0, 0, 2, -1,     /* Folk Preset */
                                        4, 1, 9, 3, 0,      /* Heavy Metal Preset */
                                        5, 3, 0, 1, 3,      /* Hip Hop Preset */
                                        4, 2, -2, 2, 5,     /* Jazz Preset */
                                       -1, 2, 5, 1, -2,     /* Pop Preset */
                                        5, 3, -1, 3, 5};    /* Rock Preset */

static const PresetConfig gEqualizerPresets[] = {
                                        {"Normal"},
                                        {"Classical"},
                                        {"Dance"},
                                        {"Flat"},
                                        {"Folk"},
                                        {"Heavy Metal"},
                                        {"Hip Hop"},
                                        {"Jazz"},
                                        {"Pop"},
                                        {"Rock"}};





int EffectCreate();
int LvmBundle_init(EffectContext *pContext);
int LvmBundle_process(LVM_INT16        *pIn,
                      LVM_INT16        *pOut,
                      int              frameCount,
                      EffectContext    *pContext);

int LvmEffect_enable(EffectContext *pContext);
int LvmEffect_disable(EffectContext *pContext);
void LvmEffect_free(EffectContext *pContext);

#if __cplusplus
}  // extern "C"
#endif


#endif /*ANDROID_EFFECTBUNDLE_H_*/
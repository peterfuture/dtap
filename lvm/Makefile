.PHONY: all clean

#======================================================
#                   VERSION                      
#======================================================

DTAP_VERSION = v1.0

#======================================================
#                   SETTING                      
#======================================================

DTAP_BUNDLE = yes
DTAP_REVERB = yes

#======================================================
#                   MACRO                      
#======================================================

DT_CFLAGS += -DENABLE_BUNDLE=1
DT_CFLAGS += -DENABLE_REVERB=1

#======================================================
#                   Rules
#======================================================

#release
%.o: %.S
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.c
	@echo CC $@ 
	@$(CC) $(CFLAGS) -shared -fPIC -c -o $@ $< 

%.o: %.cpp
	$(CC) $(CXXFLAGS) -c -o $@ $<

%.o: %.m
	$(CC) $(CFLAGS) -c -o $@ $<

%-rc.o: %.rc
	$(WINDRES) -I. $< -o $@

#debug
%.debug.o: %.S
	$(CC) $(CFLAGS) -g -c -o $@ $<

%.debug.o: %.c
	@echo CC $@ 
	@$(CC) $(CFLAGS) -g -c -o $@ $< 

%.debug.o: %.cpp
	$(CXX) $(CXXFLAGS) -g -c -o $@ $<

%.debug.o: %.m
	$(CC) $(CFLAGS) -g -c -o $@ $<
#==============================================


#======================================================
#                   ENV 
#======================================================
export MAKEROOT := $(shell pwd)
LOCAL_PATH      := $(shell pwd)

AR       = ar
CC       = gcc
CXX      = g++
STRIP    = strip 

CFLAGS  += -Wall 
DT_DEBUG = -g

CFLAGS  += -I/usr/include -I/usr/local/include
LDFLAGS += -L/usr/local/lib -L/usr/lib -L./

LDFLAGS += -lpthread -lm 
LDFLAGS += $(LDFLAGS-yes)



#======================================================
# Music bundle
#======================================================

SRC_BUNDLE_FILES:= \
    StereoWidening/src/LVCS_BypassMix.c \
    StereoWidening/src/LVCS_Control.c \
    StereoWidening/src/LVCS_Equaliser.c \
    StereoWidening/src/LVCS_Init.c \
    StereoWidening/src/LVCS_Process.c \
    StereoWidening/src/LVCS_ReverbGenerator.c \
    StereoWidening/src/LVCS_StereoEnhancer.c \
    StereoWidening/src/LVCS_Tables.c \
    Bass/src/LVDBE_Control.c \
    Bass/src/LVDBE_Init.c \
    Bass/src/LVDBE_Process.c \
    Bass/src/LVDBE_Tables.c \
    Bundle/src/LVM_API_Specials.c \
    Bundle/src/LVM_Buffers.c \
    Bundle/src/LVM_Init.c \
    Bundle/src/LVM_Process.c \
    Bundle/src/LVM_Tables.c \
    Bundle/src/LVM_Control.c \
    SpectrumAnalyzer/src/LVPSA_Control.c \
    SpectrumAnalyzer/src/LVPSA_Init.c \
    SpectrumAnalyzer/src/LVPSA_Memory.c \
    SpectrumAnalyzer/src/LVPSA_Process.c \
    SpectrumAnalyzer/src/LVPSA_QPD_Init.c \
    SpectrumAnalyzer/src/LVPSA_QPD_Process.c \
    SpectrumAnalyzer/src/LVPSA_Tables.c \
    Eq/src/LVEQNB_CalcCoef.c \
    Eq/src/LVEQNB_Control.c \
    Eq/src/LVEQNB_Init.c \
    Eq/src/LVEQNB_Process.c \
    Eq/src/LVEQNB_Tables.c 

SRC_COMMON_FILES:= \
	Common/src/InstAlloc.c \
    Common/src/DC_2I_D16_TRC_WRA_01.c \
    Common/src/DC_2I_D16_TRC_WRA_01_Init.c \
    Common/src/FO_2I_D16F32C15_LShx_TRC_WRA_01.c \
    Common/src/FO_2I_D16F32Css_LShx_TRC_WRA_01_Init.c \
    Common/src/FO_1I_D16F16C15_TRC_WRA_01.c \
    Common/src/FO_1I_D16F16Css_TRC_WRA_01_Init.c \
    Common/src/BP_1I_D16F32C30_TRC_WRA_01.c \
    Common/src/BP_1I_D16F16C14_TRC_WRA_01.c \
    Common/src/BP_1I_D32F32C30_TRC_WRA_02.c \
    Common/src/BP_1I_D16F16Css_TRC_WRA_01_Init.c \
    Common/src/BP_1I_D16F32Cll_TRC_WRA_01_Init.c \
    Common/src/BP_1I_D32F32Cll_TRC_WRA_02_Init.c \
    Common/src/BQ_2I_D32F32Cll_TRC_WRA_01_Init.c \
    Common/src/BQ_2I_D32F32C30_TRC_WRA_01.c \
    Common/src/BQ_2I_D16F32C15_TRC_WRA_01.c \
    Common/src/BQ_2I_D16F32C14_TRC_WRA_01.c \
    Common/src/BQ_2I_D16F32C13_TRC_WRA_01.c \
    Common/src/BQ_2I_D16F32Css_TRC_WRA_01_init.c \
    Common/src/BQ_2I_D16F16C15_TRC_WRA_01.c \
    Common/src/BQ_2I_D16F16C14_TRC_WRA_01.c \
    Common/src/BQ_2I_D16F16Css_TRC_WRA_01_Init.c \
    Common/src/BQ_1I_D16F16C15_TRC_WRA_01.c \
    Common/src/BQ_1I_D16F16Css_TRC_WRA_01_Init.c \
    Common/src/BQ_1I_D16F32C14_TRC_WRA_01.c \
    Common/src/BQ_1I_D16F32Css_TRC_WRA_01_init.c \
    Common/src/PK_2I_D32F32C30G11_TRC_WRA_01.c \
    Common/src/PK_2I_D32F32C14G11_TRC_WRA_01.c \
    Common/src/PK_2I_D32F32CssGss_TRC_WRA_01_Init.c \
    Common/src/PK_2I_D32F32CllGss_TRC_WRA_01_Init.c \
    Common/src/Int16LShiftToInt32_16x32.c \
    Common/src/From2iToMono_16.c \
    Common/src/Copy_16.c \
    Common/src/MonoTo2I_16.c \
    Common/src/LoadConst_16.c \
    Common/src/dB_to_Lin32.c \
    Common/src/Shift_Sat_v16xv16.c \
    Common/src/Abs_32.c \
    Common/src/Int32RShiftToInt16_Sat_32x16.c \
    Common/src/From2iToMono_32.c \
    Common/src/mult3s_16x16.c \
    Common/src/NonLinComp_D16.c \
    Common/src/DelayMix_16x16.c \
    Common/src/MSTo2i_Sat_16x16.c \
    Common/src/From2iToMS_16x16.c \
    Common/src/Mac3s_Sat_16x16.c \
    Common/src/Add2_Sat_16x16.c \
    Common/src/LVC_MixSoft_1St_2i_D16C31_SAT.c \
    Common/src/LVC_MixSoft_1St_D16C31_SAT.c \
    Common/src/LVC_Mixer_VarSlope_SetTimeConstant.c \
    Common/src/LVC_Mixer_SetTimeConstant.c \
    Common/src/LVC_Mixer_SetTarget.c \
    Common/src/LVC_Mixer_GetTarget.c \
    Common/src/LVC_Mixer_Init.c \
    Common/src/LVC_Core_MixHard_1St_2i_D16C31_SAT.c \
    Common/src/LVC_Core_MixSoft_1St_2i_D16C31_WRA.c \
    Common/src/LVC_Core_MixInSoft_D16C31_SAT.c \
    Common/src/LVC_Mixer_GetCurrent.c \
    Common/src/LVC_MixSoft_2St_D16C31_SAT.c \
    Common/src/LVC_Core_MixSoft_1St_D16C31_WRA.c \
    Common/src/LVC_Core_MixHard_2St_D16C31_SAT.c \
    Common/src/LVC_MixInSoft_D16C31_SAT.c \
    Common/src/AGC_MIX_VOL_2St1Mon_D32_WRA.c \
    Common/src/LVM_Timer.c \
    Common/src/LVM_Timer_Init.c


DIRS += .\
		Common/src/ \
		StereoWidening/src \
		Bundle/src \
		Eq/src \
		SpectrumAnalyzer/src \
		Base/src

BUNDLE_C_INCLUDES += \
    -I$(LOCAL_PATH)/Eq/lib \
    -I$(LOCAL_PATH)/Eq/src \
    -I$(LOCAL_PATH)/Bass/lib \
    -I$(LOCAL_PATH)/Bass/src \
    -I$(LOCAL_PATH)/Common/lib \
    -I$(LOCAL_PATH)/Common/src \
    -I$(LOCAL_PATH)/Bundle/lib \
    -I$(LOCAL_PATH)/Bundle/src \
    -I$(LOCAL_PATH)/SpectrumAnalyzer/lib \
    -I$(LOCAL_PATH)/SpectrumAnalyzer/src \
    -I$(LOCAL_PATH)/StereoWidening/src \
    -I$(LOCAL_PATH)/StereoWidening/lib

SRC_BUNDLE_FILES += $(SRC_COMMON_FILES)
CFLAGS += $(BUNDLE_C_INCLUDES)

BUNDLE_TARGET:= libbundle.so
OBJS_BUNDLE_RELEASE += $(addsuffix .o, $(basename $(SRC_BUNDLE_FILES)))

#=========================================================


#======================================================
# Reverb library
#======================================================


SRC_REVERB_FILES:= \
    Reverb/src/LVREV_ApplyNewSettings.c \
    Reverb/src/LVREV_ClearAudioBuffers.c \
    Reverb/src/LVREV_GetControlParameters.c \
    Reverb/src/LVREV_GetInstanceHandle.c \
    Reverb/src/LVREV_GetMemoryTable.c \
    Reverb/src/LVREV_Process.c \
    Reverb/src/LVREV_SetControlParameters.c \
    Reverb/src/LVREV_Tables.c \
    Common/src/Abs_32.c \
    Common/src/InstAlloc.c \
    Common/src/LoadConst_16.c \
    Common/src/LoadConst_32.c \
    Common/src/From2iToMono_32.c \
    Common/src/Mult3s_32x16.c \
    Common/src/FO_1I_D32F32C31_TRC_WRA_01.c \
    Common/src/FO_1I_D32F32Cll_TRC_WRA_01_Init.c \
    Common/src/DelayAllPass_Sat_32x16To32.c \
    Common/src/Copy_16.c \
    Common/src/Mac3s_Sat_32x16.c \
    Common/src/DelayWrite_32.c \
    Common/src/Shift_Sat_v32xv32.c \
    Common/src/Add2_Sat_32x32.c \
    Common/src/JoinTo2i_32x32.c \
    Common/src/MonoTo2I_32.c \
    Common/src/LVM_FO_HPF.c \
    Common/src/LVM_FO_LPF.c \
    Common/src/LVM_Polynomial.c \
    Common/src/LVM_Power10.c \
    Common/src/LVM_GetOmega.c \
    Common/src/MixSoft_2St_D32C31_SAT.c \
    Common/src/MixSoft_1St_D32C31_WRA.c \
    Common/src/MixInSoft_D32C31_SAT.c \
    Common/src/LVM_Mixer_TimeConstant.c \
    Common/src/Core_MixHard_2St_D32C31_SAT.c \
    Common/src/Core_MixSoft_1St_D32C31_WRA.c \
    Common/src/Core_MixInSoft_D32C31_SAT.c

DIRS += .\
		Reverb/src/ 

REVERB_C_INCLUDES += \
    -I$(LOCAL_PATH)/Reverb/lib \
    -I$(LOCAL_PATH)/Reverb/src \
    -I$(LOCAL_PATH)/Common/lib \
    -I$(LOCAL_PATH)/Common/src

SRC_REVERB_FILES += $(SRC_COMMON_FILES)
CFLAGS += $(REVERB_C_INCLUDES)

REVERB_TARGET:= libreverb.so
OBJS_REVERB_RELEASE += $(addsuffix .o, $(basename $(SRC_REVERB_FILES)))

DTAP_TARGET += $(BUNDLE_TARGET)
DTAP_TARGET += $(REVERB_TARGET)

all: $(DTAP_TARGET)
	@echo =====================================================
	@echo build $(DTAP_TARGET) done
	@echo =====================================================

$(BUNDLE_TARGET): $(OBJS_BUNDLE_RELEASE)
	@$(CC) -shared -fPIC -o $@ $^ $(LDFLAGS)
	@echo =====================================================
	@echo build $@ done
	@echo =====================================================

$(REVERB_TARGET):$(OBJS_REVERB_RELEASE)
	@$(CC) -shared -fPIC -o $@ $^ $(LDFLAGS)
	@echo build $@ done
	@echo =====================================================


ADDSUFFIXES  = $(foreach suf,$(1),$(addsuffix $(suf),$(2)))
ALL_DIRS     = $(call ADDSUFFIXES,$(1),$(DIRS))

clean:
	-rm -f $(call ALL_DIRS,/*.o /*.so /*.a /*.d /*.a /*.ho /*~ /*.exe)

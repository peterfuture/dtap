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
# dtap
#======================================================

LIB_SRCS         += dtap.c
LIB_C_INCLUDES   += -Iinclude
CFLAGS           += $(LIB_C_INCLUDES)
LIB_TARGET       := libdtap.so
OBJS_LIB_RELEASE += $(addsuffix .o, $(basename $(LIB_SRCS)))
DIRS             += ./

#=========================================================

#======================================================
# test
#======================================================

EXE_SRCS         += main.c
EXE_C_INCLUDES   += -Iinclude
CFLAGS           += $(EXE_C_INCLUDES)
EXE_TARGET       := test.exe
OBJS_EXE_RELEASE += $(addsuffix .o, $(basename $(EXE_SRCS)))
DIRS             += ./

#=========================================================

DTAP_TARGET += $(LIB_TARGET)
DTAP_TARGET += $(EXE_TARGET)

all: $(DTAP_TARGET)
	@echo =====================================================
	@echo build $(DTAP_TARGET) done
	@echo =====================================================

$(LIB_TARGET): $(OBJS_LIB_RELEASE)
	@$(CC) -shared -fPIC -o $@ $^ $(LDFLAGS)
	@echo =====================================================
	@echo build $@ done
	@echo =====================================================

$(EXE_TARGET): $(OBJS_EXE_RELEASE)
	@$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo =====================================================
	@echo build $@ done
	@echo =====================================================

ADDSUFFIXES  = $(foreach suf,$(1),$(addsuffix $(suf),$(2)))
ALL_DIRS     = $(call ADDSUFFIXES,$(1),$(DIRS))

clean:
	-rm -f $(call ALL_DIRS,/*.o /*.so /*.a /*.d /*.a /*.ho /*~ /*.exe)

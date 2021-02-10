# cxl-sim Makefile

DEBUG?=0
TRACE?=0

ifeq ($(DEBUG),1)
	CXXFLAGS = -Wall -DDEBUG
else
	CXXFLAGS = -Wall
endif

ifeq ($(TRACE),1)
	CXXFLAGS += -DTRACING_ON=1
endif

include ../../version_detection.mk

ifeq ($(GNUC_CPP0X), 1)
    CXXFLAGS += -std=c++0x
endif

ifneq ($(DEBUG),1)
	OPTFLAGS += -O3
else
	CXXFLAGS += 
endif

CXXFLAGS += -I$(CUDA_INSTALL_PATH)/include

POWER_FLAGS=
ifneq ($(GPGPUSIM_POWER_MODEL),)
	POWER_FLAGS = -I$(GPGPUSIM_POWER_MODEL) -DGPGPUSIM_POWER_MODEL
endif

OPTFLAGS += -g3 -fPIC

CPP = g++ $(SNOW)
OEXT = o

OUTPUT_DIR=$(SIM_OBJ_FILES_DIR)/ramulator

SRCS = $(shell ls *.cc)

EXCLUDES = 

ifeq ($(GPGPUSIM_POWER_MODEL), )
EXCLUDES += power_interface.cc
endif

CSRCS = $(filter-out $(EXCLUDES), $(SRCS))

OBJS = $(CSRCS:%.cc=$(OUTPUT_DIR)/%.$(OEXT))
 
libgpu_uarch_sim.a:$(OBJS)
	ar rcs  $(OUTPUT_DIR)/libgpu_uarch_sim.a $(OBJS)

$(OUTPUT_DIR)/Makefile.makedepend: depend

depend:
	touch $(OUTPUT_DIR)/Makefile.makedepend
	makedepend -f$(OUTPUT_DIR)/Makefile.makedepend -p$(OUTPUT_DIR)/ $(CSRCS) 2> /dev/null

$(OUTPUT_DIR)/%.$(OEXT): %.cc
	$(CPP) $(OPTFLAGS) $(CXXFLAGS) -fPIC -std=c++11 -g -Wall -o $(OUTPUT_DIR)/$*.$(OEXT) -c $*.cc

clean:
	rm -f *.o core *~ *.a 
	rm -f Makefile.makedepend Makefile.makedepend.bak

$(OUTPUT_DIR)/option_parser.$(OEXT): option_parser.h

$(OUTPUT_DIR)/dram_sched.$(OEXT): $(OUTPUT_DIR)/../cuda-sim/ptx.tab.h

$(OUTPUT_DIR)/../cuda-sim/ptx.tab.h:
	make -C ../cuda-sim/ $(OUTPUT_DIR)/../cuda-sim/ptx.tab.c

include $(OUTPUT_DIR)/Makefile.makedepend


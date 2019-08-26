###########################################################################
## Adjust the following two variables to match your project.             ##
###########################################################################

CC = gcc
CXX = g++

ROSE_INSTALL_PATH = /opt/compilers/ROSE
BOOST_INSTALL_PATH = /opt/libraries/BOOST
POLYOPT_INSTALL_PATH = /opt/compilers/ROSE/rose-polyopt/projects/PolyOpt2
POCC_INSTALL = $(POLYOPT_INSTALL_PATH)/pocc
BARVINOK_INSTALL_PATH = /opt/libraries/barvinok-0.41
NTL_INSTALL_PATH = /opt/libraries/ntl-10.5

###########################################################################
## You should not need to change anything below this line.               ##
###########################################################################

OPTFLAGS = -g -O2

ifeq ($(DEBUG),1)
	OPTFLAGS = -g -fp-trap-all=common
endif

CFLAGS = $(OPTFLAGS) \
	-I$(BARVINOK_INSTALL_PATH)/include \
	-I$(NTL_INSTALL_PATH)/include \
	-I$(ROSE_INSTALL_PATH)/include/rose \
	-I$(BOOST_INSTALL_PATH)/include \
        -I$(POLYOPT_INSTALL_PATH)/polyopt/include \
	-I$(POCC_INSTALL)/driver/pocc/include				\
	-I$(POCC_INSTALL)/driver/pocc-utils/include			\
	-I$(POCC_INSTALL)/optimizers/install-letsee/include		\
	-I$(POCC_INSTALL)/optimizers/install-pluto/include		\
	-I$(POCC_INSTALL)/optimizers/install-vectorizer/include		\
	-I$(POCC_INSTALL)/optimizers/install-parr/include		\
	-I$(POCC_INSTALL)/optimizers/install-psimdkzer/include		\
	-I$(POCC_INSTALL)/ir/install-scoplib/include			\
	-I$(POCC_INSTALL)/ir/install-scoptools/include			\
	-I$(POCC_INSTALL)/ir/install-past/include			\
	-I$(POCC_INSTALL)/ir/install-irconverter/include		\
	-I$(POCC_INSTALL)/analyzers/install-clan/include		\
	-I$(POCC_INSTALL)/analyzers/install-candl/include		\
	-I$(POCC_INSTALL)/analyzers/install-polyfeat/include		\
	-I$(POCC_INSTALL)/generators/install-cloog/include		\
	-I$(POCC_INSTALL)/generators/install-pragmatize/include		\
	-I$(POCC_INSTALL)/generators/install-clasttools/include		\
	-I$(POCC_INSTALL)/generators/install-kunroller/include		\
	-I$(POCC_INSTALL)/generators/install-rotreg/include		\
	-I$(POCC_INSTALL)/generators/install-sicgen/include		\
	-I$(POCC_INSTALL)/generators/install-ptile/include		\
	-I$(POCC_INSTALL)/math/external/install/include			\
	-I$(POCC_INSTALL)/math/install-piplib-hybrid/include		\
	-I$(POCC_INSTALL)/math/install-piplib/include			\
	-I$(POCC_INSTALL)/math/install-polylib/include			\
	-I$(POCC_INSTALL)/math/install-isl/include			\
	-I$(POCC_INSTALL)/math/install-barvinok/include			\
	-I$(POCC_INSTALL)/math/install-ntl-6.2.0/include		\
	-I$(POCC_INSTALL)/math/install-fm/include			\

LFLAGS = -L$(ROSE_INSTALL_PATH)/lib -L$(BOOST_INSTALL_PATH)/lib -lm -lrose -lboost_system -L$(POLYOPT_INSTALL_PATH)/polyopt/ -lpolyopt

OBJFILES = PolyCheck.o Statement.o Orchestrator.o CodeParser.o DependenceAnalyzer.o Dependences.o \
DebugComputer.o GeneralUtilityClass.o LiveDataComputer.o UnusedComputer.o UnusedDeclComputer.o

TARGET = PolyCheck

all: $(TARGET)

$(TARGET): $(OBJFILES)
	 $(CXX) -g -lboost_iostreams -lboost_system -L/opt/libraries/BOOST/lib \
	 -o PolyCheck PolyCheck.o Statement.o Orchestrator.o DependenceAnalyzer.o Dependences.o DebugComputer.o \
	 GeneralUtilityClass.o LiveDataComputer.o UnusedComputer.o UnusedDeclComputer.o CodeParser.o \
	 $(POLYOPT_INSTALL_PATH)/polyopt/.libs/libpolyopt.so /opt/compilers/ROSE/lib/librose.so \
	 $(BARVINOK_INSTALL_PATH)/lib/libisl.so \
     -L$(BARVINOK_INSTALL_PATH)/lib -L$(NTL_INSTALL_PATH)/lib -lbarvinok -lntl -lgmp -lpolylibgmp \
	 $(POLYOPT_INSTALL_PATH)/pocc/math/install-isl/lib/libisl.so 
	 #$(POLYOPT_INSTALL_PATH)/pocc/math/install-barvinok/lib/libbarvinok.so

%.o: %.cpp
	$(CXX) $(CFLAGS) -c -o $@ $^

clean:
	rm -f *~ $(OBJFILES) $(TARGET) 



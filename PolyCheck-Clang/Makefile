###########################################################################
## Adjust the following two variables to match your project.             ##
###########################################################################

CXX = g++

NTL_INSTALL_PATH=/Users/sriram/local/ntl
BARVINOK_INSTALL_PATH=/Users/sriram/local/barvinok-0.41
LLVM_INSTALL_PATH=/usr/local/opt/llvm/
GMP_INSTALL_PATH=  #Optional if in standard library path

###########################################################################
## You should not need to change anything below this line.               ##
###########################################################################

#CFLAGS = -fno-rtti

BINARY_FILE	= PolyCheck

LLVM_LIBS=all
LLVM_BIN_PATH = $(LLVM_INSTALL_PATH)/bin
LLVM_CONFIG_COMMAND = $(LLVM_BIN_PATH)/llvm-config --cxxflags --libs $(LLVM_LIBS); $(LLVM_BIN_PATH)/llvm-config  --ldflags
CLANG_BUILD_FLAGS = -I$(LLVM_INSTALL_PATH)/include/clang 

CLANGLIBS = \
  -lclangFrontendTool -lclangFrontend -lclangDriver \
  -lclangSerialization -lclangCodeGen -lclangParse \
  -lclangSema -lclangStaticAnalyzerFrontend \
  -lclangStaticAnalyzerCheckers -lclangStaticAnalyzerCore \
  -lclangAnalysis -lclangARCMigrate -lclangRewrite \
  -lclangEdit -lclangAST -lclangLex -lclangBasic

#-lm -lfl -lrose -lboost_system
LIBRARY_FLAGS = -L$(GMP_INSTALL_PATH)/lib -L$(BARVINOK_INSTALL_PATH)/lib -L$(NTL_INSTALL_PATH)/lib -lbarvinok -lpet -lisl -lntl -lgmp -lpolylibgmp $(CFLAGS) $(CLANG_BUILD_FLAGS) $(CLANGLIBS) `$(LLVM_CONFIG_COMMAND)`
CXXFLAGS = -Wno-unused-variable -g -O0 $(CFLAGS) -I . -I utilities/ -I$(GMP_INSTALL_PATH)/include -I$(BARVINOK_INSTALL_PATH)/include -I$(NTL_INSTALL_PATH)/include $(CLANG_BUILD_FLAGS) `$(LLVM_CONFIG_COMMAND)`

TEMP0_FILES = $(SOURCE_FILES:.cpp=.o)
TEMP1_FILES = $(TEMP0_FILES:.C=.o)
OBJECT_FILES = $(TEMP1_FILES:.cc=.o)
OTHER_BINARIES = 


$(BINARY_FILE):	PolyCheck.o
			$(CXX) -o $(BINARY_FILE) $(LDFLAGS) PolyCheck.o $(OTHER_BINARIES) $(LIBRARY_FLAGS)

PolyCheck.o: PolyCheck.cpp PolyCheck.hpp islw.hpp util.hpp
		$(CXX) -c $(CXXFLAGS) PolyCheck.cpp

all		:	$(BINARY_FILE)

.cpp.o          :
			$(CXX) -c $(CXXFLAGS) -o $@ $<
.C.o            :
			$(CXX) -c $(CXXFLAGS) -o $@ $<
.cc.o           :
			$(CXX) -c $(CXXFLAGS) -o $@ $<

clean		:
			rm -f *.o
disclean 	:
			rm $(BINARY_FILE)



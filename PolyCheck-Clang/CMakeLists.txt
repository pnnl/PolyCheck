cmake_minimum_required(VERSION 3.6)
project(PolyCheck)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

execute_process(COMMAND ${LLVM_INSTALL_PATH}/bin/llvm-config --ldflags --libs
        OUTPUT_VARIABLE LLVM_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(COMMAND ${LLVM_INSTALL_PATH}/bin/llvm-config --cxxflags
        OUTPUT_VARIABLE LLVM_FLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)

string(REPLACE "\n" " " LLVM_LIBS ${LLVM_LIBS})

set(CLANG_LIBS
-Wl,--start-group
-lclangFrontendTool -lclangFrontend -lclangDriver 
-lclangSerialization -lclangCodeGen -lclangParse 
-lclangSema -lclangStaticAnalyzerFrontend 
-lclangStaticAnalyzerCheckers -lclangStaticAnalyzerCore 
-lclangAnalysis -lclangARCMigrate -lclangRewrite 
-lclangEdit -lclangAST -lclangLex -lclangBasic
-Wl,--end-group
)

set(POLYCHECK_SRCS PolyCheck.cpp Orchestrator.cpp Statement.cpp DependenceAnalyzer.cpp
 Dependences.cpp GeneralUtilityClass.cpp CodeParser.cpp UnusedComputer.cpp 
Processor.cpp LiveDataComputer.cpp DebugComputer.cpp UnusedDeclComputer.cpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LLVM_FLAGS} -fno-rtti")
add_executable(PolyCheck ${POLYCHECK_SRCS})
target_include_directories(PolyCheck SYSTEM PUBLIC ${PROJECT_SOURCE_DIR}
${BARVINOK_INSTALL_PATH}/include ${NTL_INSTALL_PATH}/include ${LLVM_INSTALL_PATH}/include)
target_link_libraries(PolyCheck ${CLANG_LIBS} 
-L${NTL_INSTALL_PATH}/lib -L${BARVINOK_INSTALL_PATH}/lib 
-lbarvinok -lpet -lisl -lntl -lgmp -lpolylibgmp -lpthread -lz ${LLVM_LIBS})
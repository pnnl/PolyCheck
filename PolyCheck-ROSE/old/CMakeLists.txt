cmake_minimum_required(VERSION 3.7 FATAL_ERROR)
project(POLYCHECK-ROSE C CXX)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if( NOT ROSE_INSTALL_PATH )
    message(FATAL_ERROR "ROSE_INSTALL_PATH not specified")
endif()

if( NOT BOOST_INSTALL_PATH )
    message(FATAL_ERROR "BOOST_INSTALL_PATH not specified")
endif()

if( NOT POLYOPT_INSTALL_PATH )
    message(FATAL_ERROR "POLYOPT_INSTALL_PATH not specified")
endif()

if( NOT POCC_INSTALL_PATH )
    set(POCC_INSTALL_PATH ${POLYOPT_INSTALL_PATH}/pocc)
endif()

if (NOT EXISTS ${POCC_INSTALL_PATH})
    message(FATAL_ERROR "POCC not found! Tried looking for POCC at: ${POCC_INSTALL_PATH}
            Please specify POCC_INSTALL_PATH")
endif()

if( NOT NTL_INSTALL_PATH )
    message(FATAL_ERROR "NTL_INSTALL_PATH not specified")
endif()

if( NOT BARVINOK_INSTALL_PATH )
    message(FATAL_ERROR "BARVINOK_INSTALL_PATH not specified")
endif()

message(STATUS "ROSE install path set to: ${ROSE_INSTALL_PATH}")
message(STATUS "BOOST install path set to: ${BOOST_INSTALL_PATH}")
message(STATUS "Polyopt2 install path set to: ${POLYOPT_INSTALL_PATH}")
message(STATUS "POCC install path set to: ${POCC_INSTALL_PATH}")
message(STATUS "NTL install path set to: ${NTL_INSTALL_PATH}")
message(STATUS "BARVINOK install path set to: ${BARVINOK_INSTALL_PATH}")

set(POLYCHECK_SOURCES
    PolyCheck.cpp
    CodeParser.cpp
    Statement.cpp
    Orchestrator.cpp
    DependenceAnalyzer.cpp
    Dependences.cpp
    DebugComputer.cpp
    GeneralUtilityClass.cpp
    LiveDataComputer.cpp
    UnusedComputer.cpp
    UnusedDeclComputer.cpp)

set(POLYCHECK_INCLUDES
    ${PROJECT_SOURCE_DIR}
    ${BARVINOK_INSTALL_PATH}/include 
    ${NTL_INSTALL_PATH}/include
    ${ROSE_INSTALL_PATH}/include/rose
    ${BOOST_INSTALL_PATH}/include
    ${POLYOPT_INSTALL_PATH}/polyopt/include
    ${POCC_INSTALL_PATH}/driver/pocc/include
    ${POCC_INSTALL_PATH}/driver/pocc-utils/include
    ${POCC_INSTALL_PATH}/optimizers/install-letsee/include
    ${POCC_INSTALL_PATH}/optimizers/install-pluto/include
    ${POCC_INSTALL_PATH}/optimizers/install-vectorizer/include
    ${POCC_INSTALL_PATH}/optimizers/install-parr/include
    ${POCC_INSTALL_PATH}/optimizers/install-psimdkzer/include
    ${POCC_INSTALL_PATH}/ir/install-scoplib/include
    ${POCC_INSTALL_PATH}/ir/install-scoptools/include
    ${POCC_INSTALL_PATH}/ir/install-past/include
    ${POCC_INSTALL_PATH}/ir/install-irconverter/include
    ${POCC_INSTALL_PATH}/analyzers/install-clan/include
    ${POCC_INSTALL_PATH}/analyzers/install-candl/include
    ${POCC_INSTALL_PATH}/analyzers/install-polyfeat/include
    ${POCC_INSTALL_PATH}/generators/install-cloog/include
    ${POCC_INSTALL_PATH}/generators/install-pragmatize/include
    ${POCC_INSTALL_PATH}/generators/install-clasttools/include
    ${POCC_INSTALL_PATH}/generators/install-kunroller/include
    ${POCC_INSTALL_PATH}/generators/install-rotreg/include
    ${POCC_INSTALL_PATH}/generators/install-sicgen/include
    ${POCC_INSTALL_PATH}/generators/install-ptile/include
    ${POCC_INSTALL_PATH}/math/external/install/include
    ${POCC_INSTALL_PATH}/math/install-piplib-hybrid/include
    ${POCC_INSTALL_PATH}/math/install-piplib/include
    ${POCC_INSTALL_PATH}/math/install-polylib/include
    ${POCC_INSTALL_PATH}/math/install-isl/include
    ${POCC_INSTALL_PATH}/math/install-barvinok/include
    ${POCC_INSTALL_PATH}/math/install-ntl-6.2.0/include
    ${POCC_INSTALL_PATH}/math/install-fm/include
    )
add_executable(PolyCheck ${POLYCHECK_SOURCES})
#add_dependencies(PolyCheck X Y Z)
target_include_directories(PolyCheck SYSTEM PUBLIC ${POLYCHECK_INCLUDES})
target_compile_options(PolyCheck PUBLIC -fPIC)
target_link_libraries(PolyCheck
        ${POLYOPT_INSTALL_PATH}/polyopt/.libs/libpolyopt.so
        ${ROSE_INSTALL_PATH}/lib/librose.so
        ${BOOST_INSTALL_PATH}/lib/libboost_system.so
        ${BARVINOK_INSTALL_PATH}/lib/libisl.so
        -L${BARVINOK_INSTALL_PATH}/lib -L${NTL_INSTALL_PATH}/lib -lbarvinok -lntl -lgmp -lpolylibgmp
        ${POLYOPT_INSTALL_PATH}/pocc/math/install-isl/lib/libisl.so
        )
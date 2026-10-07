cmake_minimum_required(VERSION 3.20)
project(2014_AE VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# The supplied Adobe After Effects SDK in this repository is located at:
# extern/AfterEffectsSDK/Examples/{Headers,Resources,Util,GPUUtils}
set(AE_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern/AfterEffectsSDK" CACHE PATH "Path to official Adobe After Effects SDK")
set(AE_SDK_EXAMPLES_DIR "${AE_SDK_DIR}/Examples" CACHE PATH "Path to Adobe After Effects SDK Examples root")

# 1. Core Image Processing Library (Camera Model + 2014 Filter + Sensor Grain Engine)
add_library(2014_core STATIC
    src/camera_profiles.cpp
    src/filter_engine.cpp
    src/grain_engine.cpp
    src/pipeline_core.cpp
)
target_include_directories(2014_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

if(MSVC)
    target_compile_definitions(2014_core PRIVATE _CRT_SECURE_NO_WARNINGS=1)
    target_compile_options(2014_core PRIVATE /W3 /EHsc /fp:precise /O2)
else()
    target_compile_options(2014_core PRIVATE -Wall -Wextra -O2)
endif()

# 2. Automated Core Verification Test Suite
enable_testing()
add_executable(2014_tests tests/test_engine.cpp)
target_link_libraries(2014_tests PRIVATE 2014_core)
add_test(NAME 2014_core_tests COMMAND 2014_tests)

# 3. Native Adobe After Effects 23.2.1 Plug-in Target (2014.aex)
# Primary check: extern/AfterEffectsSDK/Examples/Headers/AE_Effect.h
set(AE_SDK_BASE "")
if(EXISTS "${AE_SDK_EXAMPLES_DIR}/Headers/AE_Effect.h")
    set(AE_SDK_BASE "${AE_SDK_EXAMPLES_DIR}")
elseif(EXISTS "${AE_SDK_DIR}/Headers/AE_Effect.h")
    set(AE_SDK_BASE "${AE_SDK_DIR}")
endif()

if(AE_SDK_BASE)
    set(AE_SDK_HEADERS_DIR "${AE_SDK_BASE}/Headers")
    set(AE_SDK_RESOURCES_DIR "${AE_SDK_BASE}/Resources")
    set(AE_SDK_UTIL_DIR "${AE_SDK_BASE}/Util")
    set(AE_SDK_GPUUTILS_DIR "${AE_SDK_BASE}/GPUUtils")

    message(STATUS "[2014] Using Adobe After Effects SDK base: ${AE_SDK_BASE}")
    message(STATUS "[2014]   Headers:   ${AE_SDK_HEADERS_DIR}")
    message(STATUS "[2014]   Resources: ${AE_SDK_RESOURCES_DIR}")
    message(STATUS "[2014]   Util:      ${AE_SDK_UTIL_DIR}")
    if(EXISTS "${AE_SDK_GPUUTILS_DIR}")
        message(STATUS "[2014]   GPUUtils:  ${AE_SDK_GPUUTILS_DIR}")
    endif()

    set(PIPL_RC_FILE "")
    if(WIN32 AND MSVC)
        find_program(PIPL_TOOL
            NAMES PiPLtool.exe PiPLTool.exe
            PATHS
                "${AE_SDK_RESOURCES_DIR}"
                "${AE_SDK_EXAMPLES_DIR}/Resources"
                "${AE_SDK_DIR}/Resources"
            NO_DEFAULT_PATH
        )

        if(NOT PIPL_TOOL)
            message(FATAL_ERROR "[2014] PiPLtool.exe not found under ${AE_SDK_RESOURCES_DIR}. Cannot build genuine 2014.aex.")
        endif()

        message(STATUS "[2014]   PiPLtool:  ${PIPL_TOOL}")

        set(R_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/2014_PiPL.r")
        set(RR_INTERMEDIATE "${CMAKE_CURRENT_BINARY_DIR}/2014_PiPL.rr")
        set(RRC_INTERMEDIATE "${CMAKE_CURRENT_BINARY_DIR}/2014_PiPL.rrc")
        set(PIPL_RC_FILE "${CMAKE_CURRENT_BINARY_DIR}/2014_PiPL.rc")

        add_custom_command(
            OUTPUT "${PIPL_RC_FILE}"
            COMMAND "${CMAKE_CXX_COMPILER}" /nologo /D "MSWindows" /D "_WIN64" /D "_M_X64" /D "AE_OS_WIN" /D "AE_PROC_INTELx64" /I "${AE_SDK_HEADERS_DIR}" /I "${AE_SDK_RESOURCES_DIR}" /EP "${R_SOURCE}" > "${RR_INTERMEDIATE}"
            COMMAND "${PIPL_TOOL}" "${RR_INTERMEDIATE}" "${RRC_INTERMEDIATE}"
            COMMAND "${CMAKE_CXX_COMPILER}" /nologo /D "MSWindows" /EP "${RRC_INTERMEDIATE}" > "${PIPL_RC_FILE}"
            DEPENDS "${R_SOURCE}"
            COMMENT "Generating 2014_PiPL.rc via ${PIPL_TOOL}"
            VERBATIM
        )
    endif()

    add_library(2014_aex SHARED
        src/2014_Effect.cpp
        ${PIPL_RC_FILE}
    )

    target_compile_definitions(2014_aex PRIVATE
        Y2014_HAS_AE_SDK=1
        MSWindows=1
        WIN32=1
        _WINDOWS=1
        _WIN64=1
        AE_OS_WIN=1
        AE_PROC_INTELx64=1
        _CRT_SECURE_NO_WARNINGS=1
    )

    target_include_directories(2014_aex PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${AE_SDK_HEADERS_DIR}"
        "${AE_SDK_HEADERS_DIR}/SP"
        "${AE_SDK_HEADERS_DIR}/Win"
        "${AE_SDK_RESOURCES_DIR}"
        "${AE_SDK_UTIL_DIR}"
        "${AE_SDK_GPUUTILS_DIR}"
    )

    if(MSVC)
        target_compile_options(2014_aex PRIVATE /W3 /EHsc /fp:precise /O2)
    endif()

    target_link_libraries(2014_aex PRIVATE 2014_core)

    set_target_properties(2014_aex PROPERTIES
        OUTPUT_NAME "2014"
        PREFIX ""
        SUFFIX ".aex"
    )
else()
    if(WIN32)
        message(FATAL_ERROR "[2014] Adobe After Effects SDK not found at ${AE_SDK_EXAMPLES_DIR}/Headers/AE_Effect.h or ${AE_SDK_DIR}/Headers/AE_Effect.h.")
    else()
        message(WARNING "[2014] Non-Windows host without extern/AfterEffectsSDK/Examples/Headers/AE_Effect.h: building 2014_core and 2014_tests.")
    endif()
endif()

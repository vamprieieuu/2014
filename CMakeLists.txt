cmake_minimum_required(VERSION 3.20)
project(2014_AE VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(AE_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern/AfterEffectsSDK" CACHE PATH "Path to official Adobe After Effects SDK")

# 1. Core Image Processing Library (Camera Model + 2014 Filter + Sensor Grain Engine)
add_library(2014_core STATIC
    src/camera_profiles.cpp
    src/filter_engine.cpp
    src/grain_engine.cpp
    src/pipeline_core.cpp
)
target_include_directories(2014_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

if(MSVC)
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
# Inspect extern/AfterEffectsSDK for Headers, Resources, Util, and PiPLtool
set(AE_SDK_HEADERS_DIR "")
if(EXISTS "${AE_SDK_DIR}/Headers/AE_Effect.h")
    set(AE_SDK_HEADERS_DIR "${AE_SDK_DIR}/Headers")
elseif(EXISTS "${AE_SDK_DIR}/Examples/Headers/AE_Effect.h")
    set(AE_SDK_HEADERS_DIR "${AE_SDK_DIR}/Examples/Headers")
endif()

if(AE_SDK_HEADERS_DIR)
    get_filename_component(AE_SDK_ROOT "${AE_SDK_HEADERS_DIR}" DIRECTORY)
    message(STATUS "[2014] Found Adobe After Effects SDK root at: ${AE_SDK_ROOT}")

    set(AE_SDK_UTIL_DIR "${AE_SDK_ROOT}/Util")
    set(AE_SDK_RESOURCES_DIR "${AE_SDK_ROOT}/Resources")

    set(AE_SDK_UTIL_SOURCES "")
    if(EXISTS "${AE_SDK_UTIL_DIR}/AEGP_SuiteHandler.cpp")
        list(APPEND AE_SDK_UTIL_SOURCES "${AE_SDK_UTIL_DIR}/AEGP_SuiteHandler.cpp")
    endif()
    if(EXISTS "${AE_SDK_UTIL_DIR}/MissingSuiteError.cpp")
        list(APPEND AE_SDK_UTIL_SOURCES "${AE_SDK_UTIL_DIR}/MissingSuiteError.cpp")
    endif()
    if(EXISTS "${AE_SDK_UTIL_DIR}/Smart_Utils.cpp")
        list(APPEND AE_SDK_UTIL_SOURCES "${AE_SDK_UTIL_DIR}/Smart_Utils.cpp")
    endif()

    set(PIPL_RC_FILE "")
    if(WIN32 AND MSVC)
        find_program(PIPL_TOOL
            NAMES PiPLtool.exe PiPLTool.exe
            PATHS
                "${AE_SDK_ROOT}/Resources"
                "${AE_SDK_DIR}/Resources"
                "${AE_SDK_DIR}/Examples/Resources"
            NO_DEFAULT_PATH
        )

        if(PIPL_TOOL)
            message(STATUS "[2014] Found PiPLtool: ${PIPL_TOOL}")
            set(R_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/2014_PiPL.r")
            set(RR_INTERMEDIATE "${CMAKE_CURRENT_BINARY_DIR}/2014_PiPL.rr")
            set(RRC_INTERMEDIATE "${CMAKE_CURRENT_BINARY_DIR}/2014_PiPL.rrc")
            set(PIPL_RC_FILE "${CMAKE_CURRENT_BINARY_DIR}/2014_PiPL.rc")

            add_custom_command(
                OUTPUT "${PIPL_RC_FILE}"
                COMMAND "${CMAKE_CXX_COMPILER}" /I "${AE_SDK_HEADERS_DIR}" /I "${AE_SDK_RESOURCES_DIR}" /EP "${R_SOURCE}" > "${RR_INTERMEDIATE}"
                COMMAND "${PIPL_TOOL}" "${RR_INTERMEDIATE}" "${RRC_INTERMEDIATE}"
                COMMAND "${CMAKE_CXX_COMPILER}" /D "MSWindows" /EP "${RRC_INTERMEDIATE}" > "${PIPL_RC_FILE}"
                DEPENDS "${R_SOURCE}"
                COMMENT "Compiling 2014_PiPL.r via official Adobe PiPLtool.exe"
                VERBATIM
            )
        else()
            message(FATAL_ERROR "[2014] PiPLtool.exe is missing under ${AE_SDK_DIR}/Resources. Cannot build genuine 2014.aex without PiPLtool.exe.")
        endif()
    endif()

    add_library(2014_aex SHARED
        src/2014_Effect.cpp
        ${AE_SDK_UTIL_SOURCES}
        ${PIPL_RC_FILE}
    )

    target_compile_definitions(2014_aex PRIVATE
        Y2014_HAS_AE_SDK=1
        MSWindows=1
        WIN32=1
        _WINDOWS=1
    )

    target_include_directories(2014_aex PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${AE_SDK_HEADERS_DIR}"
        "${AE_SDK_HEADERS_DIR}/SP"
        "${AE_SDK_UTIL_DIR}"
        "${AE_SDK_RESOURCES_DIR}"
    )

    target_link_libraries(2014_aex PRIVATE 2014_core)

    set_target_properties(2014_aex PROPERTIES
        OUTPUT_NAME "2014"
        PREFIX ""
        SUFFIX ".aex"
    )
else()
    message(WARNING "[2014] Official Adobe After Effects SDK not found at ${AE_SDK_DIR}. Building 2014_core and 2014_tests only. Place the SDK in extern/AfterEffectsSDK to enable 2014.aex target.")
endif()

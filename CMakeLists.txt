cmake_minimum_required(VERSION 3.20)
project(2014_AE LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(AE_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern/AfterEffectsSDK")

add_library(2014_core STATIC
    src/camera_profiles.cpp
    src/grain_engine.cpp
)
target_include_directories(2014_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(EXISTS "${AE_SDK_DIR}/Headers")
    message(STATUS "After Effects SDK found: ${AE_SDK_DIR}")
else()
    message(WARNING "After Effects SDK not found. Add it under extern/AfterEffectsSDK before native AEX packaging.")
endif()

# The final AEX adapter/PiPL target is deliberately enabled only after the
# exact SDK layout is present. Never fabricate a successful .aex artifact.

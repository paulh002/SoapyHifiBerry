########################################################################
# Project setup -- only needed if device support is a stand-alone build
# We recommend that the support module be built in-tree with the driver.
########################################################################
cmake_minimum_required(VERSION 2.6)
project(SoapyHifiBerrySDR CXX)
enable_testing()

#select the release build type by default to get optimization flags
if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release")
   message(STATUS "Build type not specified: defaulting to release.")
endif(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "")

########################################################################
# Header and library resources needed to communicate with the device.
# These may be found within the build tree or in an external project.
########################################################################
set(MY_DEVICE_INCLUDE_DIRS "")
set(MY_DEVICE_LIBRARIES -lpthread -lSoapySDR -lliquid -lasound)


########################################################################
# build the module
########################################################################
find_package(SoapySDR CONFIG)

if (NOT SoapySDR_FOUND)
    message(WARNING "SoapySDR development files not found - skipping support")
    return()
endif ()

include_directories(${MY_DEVICE_INCLUDE_DIRS})
SOAPY_SDR_MODULE_UTIL(
    TARGET SoapyHifiBerrySDR
    SOURCES SoapyHifiBerry.cpp AudioInput.cpp AudioOutput.cpp configfile.cpp configoption.cpp RtAudio.cpp TCA9548.cpp TCA9548.h si5351.cpp SoapyHifiBerrySettings.cpp SoapyHifiBerryStreaming.cpp strlib.cpp Audiodefs.h AudioInput.h AudioOutput.h configfile.h configoption.h DataBuffer.h RtAudio.h si5351.h SoapyHifiBerry.h strlib.h
    LIBRARIES ${MY_DEVICE_LIBRARIES}
)

target_compile_definitions(SoapyHifiBerrySDR PRIVATE __LINUX_ALSA__)
target_compile_options(SoapyHifiBerrySDR PRIVATE -O3 -ggdb )
set_property(TARGET SoapyHifiBerrySDR PROPERTY CXX_STANDARD 17)


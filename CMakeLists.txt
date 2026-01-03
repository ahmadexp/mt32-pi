cmake_minimum_required(VERSION 3.16)
project(mt32-pi C CXX ASM)

# Default to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(HDMI_CONSOLE "Enable HDMI console" OFF)
set(BOARD "pi3-64" CACHE STRING "Target Board (pi2, pi3, pi3-64, pi4, pi4-64)")

# Paths
set(EXTERNAL_DIR "${CMAKE_SOURCE_DIR}/external")
set(CIRCLE_STDLIB_DIR "${EXTERNAL_DIR}/circle-stdlib")
set(CIRCLE_DIR "${CIRCLE_STDLIB_DIR}/libs/circle")
set(INIH_DIR "${EXTERNAL_DIR}/inih")

# Determine architecture and flags based on BOARD
if(BOARD STREQUAL "pi2")
    set(ARCH_FLAGS "-mcpu=cortex-a7 -marm -mfpu=neon-vfpv4 -mfloat-abi=hard")
    set(KERNEL_TARGET "kernel7")
    set(ARCH_PREFIX "arm-none-eabi")
elseif(BOARD STREQUAL "pi3")
    set(ARCH_FLAGS "-mcpu=cortex-a53 -marm -mfpu=neon-fp-armv8 -mfloat-abi=hard")
    set(KERNEL_TARGET "kernel8-32")
    set(ARCH_PREFIX "arm-none-eabi")
elseif(BOARD STREQUAL "pi3-64")
    set(ARCH_FLAGS "-mcpu=cortex-a53 -mlittle-endian")
    set(KERNEL_TARGET "kernel8")
    set(ARCH_PREFIX "aarch64-none-elf")
elseif(BOARD STREQUAL "pi4")
    set(ARCH_FLAGS "-mcpu=cortex-a72 -marm -mfpu=neon-fp-armv8 -mfloat-abi=hard")
    set(KERNEL_TARGET "kernel7l")
    set(ARCH_PREFIX "arm-none-eabi")
elseif(BOARD STREQUAL "pi4-64")
    set(ARCH_FLAGS "-mcpu=cortex-a72 -mlittle-endian")
    set(KERNEL_TARGET "kernel8-rpi4")
    set(ARCH_PREFIX "aarch64-none-elf")
else()
    message(FATAL_ERROR "Invalid BOARD: ${BOARD}")
endif()

add_compile_options(${ARCH_FLAGS} -Wall -Wextra -Wno-unused-parameter)

if(HDMI_CONSOLE)
    add_compile_definitions(HDMI_CONSOLE)
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}
    ${CIRCLE_STDLIB_DIR}/include
    ${INIH_DIR}
    # Newlib headers location - dependent on build status of circle-stdlib
    ${CIRCLE_STDLIB_DIR}/install/${ARCH_PREFIX}-none-circle/include
)

# FluidSynth and Munt includes (referencing the build dirs from Makefile logic)
set(MUNT_BUILD_DIR "${CMAKE_SOURCE_DIR}/build-munt")
set(FLUID_BUILD_DIR "${CMAKE_SOURCE_DIR}/build-fluidsynth")

include_directories(
    ${MUNT_BUILD_DIR}/include
    ${EXTERNAL_DIR}/munt/mt32emu
    ${FLUID_BUILD_DIR}/include
    ${EXTERNAL_DIR}/fluidsynth/include
)

# Source files
set(SOURCES
    src/config.cpp
    src/control/control.cpp
    src/control/mister.cpp
    src/control/rotaryencoder.cpp
    src/control/simplebuttons.cpp
    src/control/simpleencoder.cpp
    src/kernel.cpp
    src/lcd/drivers/hd44780.cpp
    src/lcd/drivers/hd44780fourbit.cpp
    src/lcd/drivers/hd44780i2c.cpp
    src/lcd/drivers/sh1106.cpp
    src/lcd/drivers/ssd1306.cpp
    src/lcd/ui.cpp
    src/main.cpp
    src/midimonitor.cpp
    src/midiparser.cpp
    src/mt32pi.cpp
    src/net/applemidi.cpp
    src/net/ftpdaemon.cpp
    src/net/ftpworker.cpp
    src/net/udpmidi.cpp
    src/pisound.cpp
    src/power.cpp
    src/rommanager.cpp
    src/soundfontmanager.cpp
    src/synth/mt32synth.cpp
    src/synth/soundfontsynth.cpp
    src/zoneallocator.cpp
    ${INIH_DIR}/ini.c
)

# Executable
add_executable(${KERNEL_TARGET}.elf ${SOURCES})

# Linking
# We strictly rely on the existing makefile ecosystem for libraries for now
set(NEWLIB_DIR "${CIRCLE_STDLIB_DIR}/install/${ARCH_PREFIX}-none-circle/lib")
target_link_directories(${KERNEL_TARGET}.elf PRIVATE
    ${NEWLIB_DIR}
    ${CIRCLE_DIR}/lib
    ${CIRCLE_DIR}/addon/fatfs
    ${CIRCLE_DIR}/addon/SDCard
    ${CIRCLE_DIR}/addon/wlan/hostap/wpa_supplicant
    ${CIRCLE_DIR}/addon/wlan
    ${CIRCLE_DIR}/lib/fs
    ${CIRCLE_DIR}/lib/input
    ${CIRCLE_DIR}/lib/net
    ${CIRCLE_DIR}/lib/sched
    ${CIRCLE_DIR}/lib/sound
    ${CIRCLE_DIR}/lib/usb
    ${MUNT_BUILD_DIR}
    ${FLUID_BUILD_DIR}/src
)

target_link_libraries(${KERNEL_TARGET}.elf PRIVATE
    m
    c
    circlenewlib
    fatfs
    sdcard
    wpa_supplicant
    wlan
    fs
    input
    circle
    net
    sched
    sound
    usb
    mt32emu
    fluidsynth
)

# Note: The actual link step for bare metal requires a custom linker script and nose start files
# which is usually handled by Circle's rules. This CMake is primarily for IDE support (IntelliSense)
# and experimental building.

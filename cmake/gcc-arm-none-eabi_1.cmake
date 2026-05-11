set(CMAKE_SYSTEM_NAME               Generic)
set(CMAKE_SYSTEM_PROCESSOR          arm)

set(CMAKE_C_COMPILER_ID GNU)
set(CMAKE_CXX_COMPILER_ID GNU)

# Some default GCC settings.
# Prefer arm-none-eabi from PATH, but also support STM32Cube bundled tools.
set(TOOLCHAIN_PREFIX                arm-none-eabi-)
set(STM32CUBE_GCC_BIN               "$ENV{LOCALAPPDATA}/stm32cube/bundles/gnu-tools-for-stm32/14.3.1+st.2/bin")

find_program(ARM_GCC_PATH
             NAMES ${TOOLCHAIN_PREFIX}gcc
             HINTS ${STM32CUBE_GCC_BIN})
find_program(ARM_GXX_PATH
             NAMES ${TOOLCHAIN_PREFIX}g++
             HINTS ${STM32CUBE_GCC_BIN})
find_program(ARM_OBJCOPY_PATH
             NAMES ${TOOLCHAIN_PREFIX}objcopy
             HINTS ${STM32CUBE_GCC_BIN})
find_program(ARM_SIZE_PATH
             NAMES ${TOOLCHAIN_PREFIX}size
             HINTS ${STM32CUBE_GCC_BIN})

if(NOT ARM_GCC_PATH OR NOT ARM_GXX_PATH)
    message(FATAL_ERROR
        "arm-none-eabi toolchain not found. Install STM32CubeCLT or add arm-none-eabi-gcc to PATH.")
endif()

set(CMAKE_C_COMPILER                ${ARM_GCC_PATH})
set(CMAKE_ASM_COMPILER              ${CMAKE_C_COMPILER})
set(CMAKE_CXX_COMPILER              ${ARM_GXX_PATH})
set(CMAKE_LINKER                    ${ARM_GXX_PATH})
set(CMAKE_OBJCOPY                   ${ARM_OBJCOPY_PATH})
set(CMAKE_SIZE                      ${ARM_SIZE_PATH})

set(CMAKE_EXECUTABLE_SUFFIX_ASM     ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_C       ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_CXX     ".elf")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# MCU specific flags
set(TARGET_FLAGS "-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard ")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${TARGET_FLAGS}")
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS} -x assembler-with-cpp -MMD -MP")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fdata-sections -ffunction-sections")

set(CMAKE_C_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_C_FLAGS_RELEASE "-Os -g0")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_CXX_FLAGS_RELEASE "-Os -g0")

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fno-rtti -fno-exceptions -fno-threadsafe-statics")

set(CMAKE_EXE_LINKER_FLAGS "${TARGET_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T \"${CMAKE_SOURCE_DIR}/STM32L476XX_FLASH.ld\"")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --specs=nano.specs")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Map=${CMAKE_PROJECT_NAME}.map -Wl,--gc-sections")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--print-memory-usage")
set(TOOLCHAIN_LINK_LIBRARIES "m")

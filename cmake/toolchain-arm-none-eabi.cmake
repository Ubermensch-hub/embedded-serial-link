# Toolchain-файл для кросс-сборки esl_core под Cortex-M4 (например, STM32F4)
# через GNU ARM Embedded Toolchain.
#
# Stub-драйверы, демо-приложение и тесты — host-only (loopback в памяти,
# stdio для логов), поэтому при кросс-сборке их нужно отключить и собрать
# только ядро:
#
#   cmake -B build-stm32 -G Ninja \
#         -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-none-eabi.cmake \
#         -DESL_BUILD_APP=OFF -DESL_BUILD_TESTS=OFF
#   cmake --build build-stm32
#
# esl_core не использует хип, исключения, RTTI и host-специфичные заголовки,
# поэтому собирается как есть — исключаются только main.cpp и тесты.

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)

# Без линкер-скрипта и startup-кода CMake не соберёт полноценный exe;
# проверка компилятора на статической библиотеке решает эту проблему.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(ESL_MCU_FLAGS "-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16")

set(CMAKE_C_FLAGS_INIT "${ESL_MCU_FLAGS} -ffunction-sections -fdata-sections")
set(CMAKE_CXX_FLAGS_INIT
    "${ESL_MCU_FLAGS} -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-threadsafe-statics")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${ESL_MCU_FLAGS} -Wl,--gc-sections --specs=nano.specs")

find_program(ESL_ARM_GCC arm-none-eabi-gcc)
if(NOT ESL_ARM_GCC)
    message(WARNING
        "arm-none-eabi-gcc не найден в PATH. Этот toolchain-файл лишь "
        "держит проект готовым к кросс-сборке; чтобы им воспользоваться, "
        "установите GNU Arm Embedded Toolchain.")
endif()

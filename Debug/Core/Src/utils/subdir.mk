################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/utils/hex_utils.c \
../Core/Src/utils/sensor_payload.c 

OBJS += \
./Core/Src/utils/hex_utils.o \
./Core/Src/utils/sensor_payload.o 

C_DEPS += \
./Core/Src/utils/hex_utils.d \
./Core/Src/utils/sensor_payload.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/utils/%.o Core/Src/utils/%.su Core/Src/utils/%.cyclo: ../Core/Src/utils/%.c Core/Src/utils/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32L073xx -c -I../Core/Inc -I../Drivers/STM32L0xx_HAL_Driver/Inc -I../Drivers/STM32L0xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L0xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-utils

clean-Core-2f-Src-2f-utils:
	-$(RM) ./Core/Src/utils/hex_utils.cyclo ./Core/Src/utils/hex_utils.d ./Core/Src/utils/hex_utils.o ./Core/Src/utils/hex_utils.su ./Core/Src/utils/sensor_payload.cyclo ./Core/Src/utils/sensor_payload.d ./Core/Src/utils/sensor_payload.o ./Core/Src/utils/sensor_payload.su

.PHONY: clean-Core-2f-Src-2f-utils


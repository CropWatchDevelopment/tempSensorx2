################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/Utils/hex_utils.c 

OBJS += \
./Core/Src/Utils/hex_utils.o 

C_DEPS += \
./Core/Src/Utils/hex_utils.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/Utils/%.o Core/Src/Utils/%.su Core/Src/Utils/%.cyclo: ../Core/Src/Utils/%.c Core/Src/Utils/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32L073xx -c -I../Core/Inc -I../Drivers/STM32L0xx_HAL_Driver/Inc -I../Drivers/STM32L0xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L0xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-Utils

clean-Core-2f-Src-2f-Utils:
	-$(RM) ./Core/Src/Utils/hex_utils.cyclo ./Core/Src/Utils/hex_utils.d ./Core/Src/Utils/hex_utils.o ./Core/Src/Utils/hex_utils.su

.PHONY: clean-Core-2f-Src-2f-Utils


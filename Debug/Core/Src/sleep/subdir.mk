################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/sleep/sleep.c 

OBJS += \
./Core/Src/sleep/sleep.o 

C_DEPS += \
./Core/Src/sleep/sleep.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/sleep/%.o Core/Src/sleep/%.su Core/Src/sleep/%.cyclo: ../Core/Src/sleep/%.c Core/Src/sleep/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32L073xx -c -I../Core/Inc -I../Drivers/STM32L0xx_HAL_Driver/Inc -I../Drivers/STM32L0xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L0xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-sleep

clean-Core-2f-Src-2f-sleep:
	-$(RM) ./Core/Src/sleep/sleep.cyclo ./Core/Src/sleep/sleep.d ./Core/Src/sleep/sleep.o ./Core/Src/sleep/sleep.su

.PHONY: clean-Core-2f-Src-2f-sleep


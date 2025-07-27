################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/battery/battery.c 

OBJS += \
./Core/Src/battery/battery.o 

C_DEPS += \
./Core/Src/battery/battery.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/battery/%.o Core/Src/battery/%.su Core/Src/battery/%.cyclo: ../Core/Src/battery/%.c Core/Src/battery/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32L073xx -c -I../Core/Inc -I../Drivers/STM32L0xx_HAL_Driver/Inc -I../Drivers/STM32L0xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L0xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-battery

clean-Core-2f-Src-2f-battery:
	-$(RM) ./Core/Src/battery/battery.cyclo ./Core/Src/battery/battery.d ./Core/Src/battery/battery.o ./Core/Src/battery/battery.su

.PHONY: clean-Core-2f-Src-2f-battery


################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/LoRaWAN/lorawan.c 

OBJS += \
./Core/Src/LoRaWAN/lorawan.o 

C_DEPS += \
./Core/Src/LoRaWAN/lorawan.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/LoRaWAN/%.o Core/Src/LoRaWAN/%.su Core/Src/LoRaWAN/%.cyclo: ../Core/Src/LoRaWAN/%.c Core/Src/LoRaWAN/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32L073xx -c -I../Core/Inc -I../Drivers/STM32L0xx_HAL_Driver/Inc -I../Drivers/STM32L0xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L0xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-LoRaWAN

clean-Core-2f-Src-2f-LoRaWAN:
	-$(RM) ./Core/Src/LoRaWAN/lorawan.cyclo ./Core/Src/LoRaWAN/lorawan.d ./Core/Src/LoRaWAN/lorawan.o ./Core/Src/LoRaWAN/lorawan.su

.PHONY: clean-Core-2f-Src-2f-LoRaWAN


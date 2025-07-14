################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/lorawan/lorawan_config.c 

OBJS += \
./Core/Src/lorawan/lorawan_config.o 

C_DEPS += \
./Core/Src/lorawan/lorawan_config.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/lorawan/%.o Core/Src/lorawan/%.su Core/Src/lorawan/%.cyclo: ../Core/Src/lorawan/%.c Core/Src/lorawan/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32L073xx -c -I../Core/Inc -I../Drivers/STM32L0xx_HAL_Driver/Inc -I../Drivers/STM32L0xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L0xx/Include -I../Drivers/CMSIS/Include -I../I-CUBE-ATC -I../Middlewares/Third_Party/NimaLTD_Driver/ATC -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-lorawan

clean-Core-2f-Src-2f-lorawan:
	-$(RM) ./Core/Src/lorawan/lorawan_config.cyclo ./Core/Src/lorawan/lorawan_config.d ./Core/Src/lorawan/lorawan_config.o ./Core/Src/lorawan/lorawan_config.su

.PHONY: clean-Core-2f-Src-2f-lorawan


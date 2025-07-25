################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/sensirion/sensirion.c \
../Core/Src/sensirion/sensirion_common.c \
../Core/Src/sensirion/sensirion_i2c.c \
../Core/Src/sensirion/sensirion_i2c_hal.c \
../Core/Src/sensirion/sht4x_i2c.c 

OBJS += \
./Core/Src/sensirion/sensirion.o \
./Core/Src/sensirion/sensirion_common.o \
./Core/Src/sensirion/sensirion_i2c.o \
./Core/Src/sensirion/sensirion_i2c_hal.o \
./Core/Src/sensirion/sht4x_i2c.o 

C_DEPS += \
./Core/Src/sensirion/sensirion.d \
./Core/Src/sensirion/sensirion_common.d \
./Core/Src/sensirion/sensirion_i2c.d \
./Core/Src/sensirion/sensirion_i2c_hal.d \
./Core/Src/sensirion/sht4x_i2c.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/sensirion/%.o Core/Src/sensirion/%.su Core/Src/sensirion/%.cyclo: ../Core/Src/sensirion/%.c Core/Src/sensirion/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32L073xx -c -I../Core/Inc -I../Drivers/STM32L0xx_HAL_Driver/Inc -I../Drivers/STM32L0xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L0xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-sensirion

clean-Core-2f-Src-2f-sensirion:
	-$(RM) ./Core/Src/sensirion/sensirion.cyclo ./Core/Src/sensirion/sensirion.d ./Core/Src/sensirion/sensirion.o ./Core/Src/sensirion/sensirion.su ./Core/Src/sensirion/sensirion_common.cyclo ./Core/Src/sensirion/sensirion_common.d ./Core/Src/sensirion/sensirion_common.o ./Core/Src/sensirion/sensirion_common.su ./Core/Src/sensirion/sensirion_i2c.cyclo ./Core/Src/sensirion/sensirion_i2c.d ./Core/Src/sensirion/sensirion_i2c.o ./Core/Src/sensirion/sensirion_i2c.su ./Core/Src/sensirion/sensirion_i2c_hal.cyclo ./Core/Src/sensirion/sensirion_i2c_hal.d ./Core/Src/sensirion/sensirion_i2c_hal.o ./Core/Src/sensirion/sensirion_i2c_hal.su ./Core/Src/sensirion/sht4x_i2c.cyclo ./Core/Src/sensirion/sht4x_i2c.d ./Core/Src/sensirion/sht4x_i2c.o ./Core/Src/sensirion/sht4x_i2c.su

.PHONY: clean-Core-2f-Src-2f-sensirion


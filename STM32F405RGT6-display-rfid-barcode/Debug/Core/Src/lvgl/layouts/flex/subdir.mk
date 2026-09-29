################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/lvgl/layouts/flex/lv_flex.c 

OBJS += \
./Core/Src/lvgl/layouts/flex/lv_flex.o 

C_DEPS += \
./Core/Src/lvgl/layouts/flex/lv_flex.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/lvgl/layouts/flex/%.o Core/Src/lvgl/layouts/flex/%.su Core/Src/lvgl/layouts/flex/%.cyclo: ../Core/Src/lvgl/layouts/flex/%.c Core/Src/lvgl/layouts/flex/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F405xx -c -I../FATFS/Target -I../FATFS/App -I../USB_HOST/App -I../USB_HOST/Target -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FatFs/src -I../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Host_Library/Class/HID/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -Oz -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-lvgl-2f-layouts-2f-flex

clean-Core-2f-Src-2f-lvgl-2f-layouts-2f-flex:
	-$(RM) ./Core/Src/lvgl/layouts/flex/lv_flex.cyclo ./Core/Src/lvgl/layouts/flex/lv_flex.d ./Core/Src/lvgl/layouts/flex/lv_flex.o ./Core/Src/lvgl/layouts/flex/lv_flex.su

.PHONY: clean-Core-2f-Src-2f-lvgl-2f-layouts-2f-flex


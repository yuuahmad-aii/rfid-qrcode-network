################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/lvgl/drivers/draw/eve/lv_draw_eve_display.c 

OBJS += \
./Core/Src/lvgl/drivers/draw/eve/lv_draw_eve_display.o 

C_DEPS += \
./Core/Src/lvgl/drivers/draw/eve/lv_draw_eve_display.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/lvgl/drivers/draw/eve/%.o Core/Src/lvgl/drivers/draw/eve/%.su Core/Src/lvgl/drivers/draw/eve/%.cyclo: ../Core/Src/lvgl/drivers/draw/eve/%.c Core/Src/lvgl/drivers/draw/eve/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F405xx -c -I../FATFS/Target -I../FATFS/App -I../USB_HOST/App -I../USB_HOST/Target -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FatFs/src -I../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Host_Library/Class/HID/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -Oz -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-lvgl-2f-drivers-2f-draw-2f-eve

clean-Core-2f-Src-2f-lvgl-2f-drivers-2f-draw-2f-eve:
	-$(RM) ./Core/Src/lvgl/drivers/draw/eve/lv_draw_eve_display.cyclo ./Core/Src/lvgl/drivers/draw/eve/lv_draw_eve_display.d ./Core/Src/lvgl/drivers/draw/eve/lv_draw_eve_display.o ./Core/Src/lvgl/drivers/draw/eve/lv_draw_eve_display.su

.PHONY: clean-Core-2f-Src-2f-lvgl-2f-drivers-2f-draw-2f-eve


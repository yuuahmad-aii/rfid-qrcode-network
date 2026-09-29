################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/lvgl/draw/espressif/ppa/lv_draw_ppa.c \
../Core/Src/lvgl/draw/espressif/ppa/lv_draw_ppa_buf.c \
../Core/Src/lvgl/draw/espressif/ppa/lv_draw_ppa_fill.c \
../Core/Src/lvgl/draw/espressif/ppa/lv_draw_ppa_img.c 

OBJS += \
./Core/Src/lvgl/draw/espressif/ppa/lv_draw_ppa.o \
./Core/Src/lvgl/draw/espressif/ppa/lv_draw_ppa_buf.o \
./Core/Src/lvgl/draw/espressif/ppa/lv_draw_ppa_fill.o \
./Core/Src/lvgl/draw/espressif/ppa/lv_draw_ppa_img.o 

C_DEPS += \
./Core/Src/lvgl/draw/espressif/ppa/lv_draw_ppa.d \
./Core/Src/lvgl/draw/espressif/ppa/lv_draw_ppa_buf.d \
./Core/Src/lvgl/draw/espressif/ppa/lv_draw_ppa_fill.d \
./Core/Src/lvgl/draw/espressif/ppa/lv_draw_ppa_img.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/lvgl/draw/espressif/ppa/%.o Core/Src/lvgl/draw/espressif/ppa/%.su Core/Src/lvgl/draw/espressif/ppa/%.cyclo: ../Core/Src/lvgl/draw/espressif/ppa/%.c Core/Src/lvgl/draw/espressif/ppa/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F405xx -c -I../FATFS/Target -I../FATFS/App -I../USB_HOST/App -I../USB_HOST/Target -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FatFs/src -I../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Host_Library/Class/HID/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -Oz -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-lvgl-2f-draw-2f-espressif-2f-ppa

clean-Core-2f-Src-2f-lvgl-2f-draw-2f-espressif-2f-ppa:
	-$(RM) ./Core/Src/lvgl/draw/espressif/ppa/lv_draw_ppa.cyclo ./Core/Src/lvgl/draw/espressif/ppa/lv_draw_ppa.d ./Core/Src/lvgl/draw/espressif/ppa/lv_draw_ppa.o ./Core/Src/lvgl/draw/espressif/ppa/lv_draw_ppa.su ./Core/Src/lvgl/draw/espressif/ppa/lv_draw_ppa_buf.cyclo ./Core/Src/lvgl/draw/espressif/ppa/lv_draw_ppa_buf.d ./Core/Src/lvgl/draw/espressif/ppa/lv_draw_ppa_buf.o ./Core/Src/lvgl/draw/espressif/ppa/lv_draw_ppa_buf.su ./Core/Src/lvgl/draw/espressif/ppa/lv_draw_ppa_fill.cyclo ./Core/Src/lvgl/draw/espressif/ppa/lv_draw_ppa_fill.d ./Core/Src/lvgl/draw/espressif/ppa/lv_draw_ppa_fill.o ./Core/Src/lvgl/draw/espressif/ppa/lv_draw_ppa_fill.su ./Core/Src/lvgl/draw/espressif/ppa/lv_draw_ppa_img.cyclo ./Core/Src/lvgl/draw/espressif/ppa/lv_draw_ppa_img.d ./Core/Src/lvgl/draw/espressif/ppa/lv_draw_ppa_img.o ./Core/Src/lvgl/draw/espressif/ppa/lv_draw_ppa_img.su

.PHONY: clean-Core-2f-Src-2f-lvgl-2f-draw-2f-espressif-2f-ppa


################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend.c \
../Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_a8.c \
../Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_al88.c \
../Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_argb8888.c \
../Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_argb8888_premultiplied.c \
../Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_i1.c \
../Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_l8.c \
../Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_rgb565.c \
../Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_rgb565_swapped.c \
../Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_rgb888.c 

OBJS += \
./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend.o \
./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_a8.o \
./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_al88.o \
./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_argb8888.o \
./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_argb8888_premultiplied.o \
./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_i1.o \
./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_l8.o \
./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_rgb565.o \
./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_rgb565_swapped.o \
./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_rgb888.o 

C_DEPS += \
./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend.d \
./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_a8.d \
./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_al88.d \
./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_argb8888.d \
./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_argb8888_premultiplied.d \
./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_i1.d \
./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_l8.d \
./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_rgb565.d \
./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_rgb565_swapped.d \
./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_rgb888.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/lvgl/draw/sw/blend/%.o Core/Src/lvgl/draw/sw/blend/%.su Core/Src/lvgl/draw/sw/blend/%.cyclo: ../Core/Src/lvgl/draw/sw/blend/%.c Core/Src/lvgl/draw/sw/blend/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F405xx -c -I../FATFS/Target -I../FATFS/App -I../USB_HOST/App -I../USB_HOST/Target -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FatFs/src -I../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Host_Library/Class/HID/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -Oz -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-lvgl-2f-draw-2f-sw-2f-blend

clean-Core-2f-Src-2f-lvgl-2f-draw-2f-sw-2f-blend:
	-$(RM) ./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend.cyclo ./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend.d ./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend.o ./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend.su ./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_a8.cyclo ./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_a8.d ./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_a8.o ./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_a8.su ./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_al88.cyclo ./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_al88.d ./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_al88.o ./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_al88.su ./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_argb8888.cyclo ./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_argb8888.d ./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_argb8888.o ./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_argb8888.su ./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_argb8888_premultiplied.cyclo ./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_argb8888_premultiplied.d ./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_argb8888_premultiplied.o ./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_argb8888_premultiplied.su ./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_i1.cyclo ./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_i1.d ./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_i1.o ./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_i1.su ./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_l8.cyclo ./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_l8.d ./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_l8.o ./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_l8.su ./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_rgb565.cyclo ./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_rgb565.d ./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_rgb565.o ./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_rgb565.su ./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_rgb565_swapped.cyclo ./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_rgb565_swapped.d ./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_rgb565_swapped.o ./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_rgb565_swapped.su ./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_rgb888.cyclo ./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_rgb888.d ./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_rgb888.o ./Core/Src/lvgl/draw/sw/blend/lv_draw_sw_blend_to_rgb888.su

.PHONY: clean-Core-2f-Src-2f-lvgl-2f-draw-2f-sw-2f-blend


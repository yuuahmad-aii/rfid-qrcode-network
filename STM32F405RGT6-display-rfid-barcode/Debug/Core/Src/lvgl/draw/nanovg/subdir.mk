################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/lvgl/draw/nanovg/lv_draw_nanovg.c \
../Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_3d.c \
../Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_arc.c \
../Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_border.c \
../Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_box_shadow.c \
../Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_fill.c \
../Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_grad.c \
../Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_image.c \
../Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_label.c \
../Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_layer.c \
../Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_line.c \
../Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_mask_rect.c \
../Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_triangle.c \
../Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_vector.c \
../Core/Src/lvgl/draw/nanovg/lv_nanovg_fbo_cache.c \
../Core/Src/lvgl/draw/nanovg/lv_nanovg_image_cache.c \
../Core/Src/lvgl/draw/nanovg/lv_nanovg_utils.c 

OBJS += \
./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg.o \
./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_3d.o \
./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_arc.o \
./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_border.o \
./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_box_shadow.o \
./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_fill.o \
./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_grad.o \
./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_image.o \
./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_label.o \
./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_layer.o \
./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_line.o \
./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_mask_rect.o \
./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_triangle.o \
./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_vector.o \
./Core/Src/lvgl/draw/nanovg/lv_nanovg_fbo_cache.o \
./Core/Src/lvgl/draw/nanovg/lv_nanovg_image_cache.o \
./Core/Src/lvgl/draw/nanovg/lv_nanovg_utils.o 

C_DEPS += \
./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg.d \
./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_3d.d \
./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_arc.d \
./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_border.d \
./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_box_shadow.d \
./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_fill.d \
./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_grad.d \
./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_image.d \
./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_label.d \
./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_layer.d \
./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_line.d \
./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_mask_rect.d \
./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_triangle.d \
./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_vector.d \
./Core/Src/lvgl/draw/nanovg/lv_nanovg_fbo_cache.d \
./Core/Src/lvgl/draw/nanovg/lv_nanovg_image_cache.d \
./Core/Src/lvgl/draw/nanovg/lv_nanovg_utils.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/lvgl/draw/nanovg/%.o Core/Src/lvgl/draw/nanovg/%.su Core/Src/lvgl/draw/nanovg/%.cyclo: ../Core/Src/lvgl/draw/nanovg/%.c Core/Src/lvgl/draw/nanovg/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F405xx -c -I../FATFS/Target -I../FATFS/App -I../USB_HOST/App -I../USB_HOST/Target -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FatFs/src -I../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Host_Library/Class/HID/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -Oz -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-lvgl-2f-draw-2f-nanovg

clean-Core-2f-Src-2f-lvgl-2f-draw-2f-nanovg:
	-$(RM) ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg.cyclo ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg.d ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg.o ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg.su ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_3d.cyclo ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_3d.d ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_3d.o ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_3d.su ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_arc.cyclo ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_arc.d ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_arc.o ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_arc.su ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_border.cyclo ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_border.d ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_border.o ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_border.su ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_box_shadow.cyclo ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_box_shadow.d ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_box_shadow.o ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_box_shadow.su ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_fill.cyclo ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_fill.d ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_fill.o ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_fill.su ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_grad.cyclo ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_grad.d ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_grad.o ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_grad.su ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_image.cyclo ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_image.d ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_image.o ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_image.su ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_label.cyclo ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_label.d ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_label.o ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_label.su ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_layer.cyclo ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_layer.d ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_layer.o ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_layer.su ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_line.cyclo ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_line.d ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_line.o ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_line.su ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_mask_rect.cyclo ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_mask_rect.d ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_mask_rect.o ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_mask_rect.su ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_triangle.cyclo ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_triangle.d ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_triangle.o ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_triangle.su ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_vector.cyclo ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_vector.d ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_vector.o ./Core/Src/lvgl/draw/nanovg/lv_draw_nanovg_vector.su ./Core/Src/lvgl/draw/nanovg/lv_nanovg_fbo_cache.cyclo ./Core/Src/lvgl/draw/nanovg/lv_nanovg_fbo_cache.d ./Core/Src/lvgl/draw/nanovg/lv_nanovg_fbo_cache.o ./Core/Src/lvgl/draw/nanovg/lv_nanovg_fbo_cache.su ./Core/Src/lvgl/draw/nanovg/lv_nanovg_image_cache.cyclo ./Core/Src/lvgl/draw/nanovg/lv_nanovg_image_cache.d ./Core/Src/lvgl/draw/nanovg/lv_nanovg_image_cache.o ./Core/Src/lvgl/draw/nanovg/lv_nanovg_image_cache.su ./Core/Src/lvgl/draw/nanovg/lv_nanovg_utils.cyclo ./Core/Src/lvgl/draw/nanovg/lv_nanovg_utils.d ./Core/Src/lvgl/draw/nanovg/lv_nanovg_utils.o ./Core/Src/lvgl/draw/nanovg/lv_nanovg_utils.su

.PHONY: clean-Core-2f-Src-2f-lvgl-2f-draw-2f-nanovg


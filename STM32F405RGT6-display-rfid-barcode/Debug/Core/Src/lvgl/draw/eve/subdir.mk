################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/lvgl/draw/eve/lv_draw_eve.c \
../Core/Src/lvgl/draw/eve/lv_draw_eve_arc.c \
../Core/Src/lvgl/draw/eve/lv_draw_eve_fill.c \
../Core/Src/lvgl/draw/eve/lv_draw_eve_image.c \
../Core/Src/lvgl/draw/eve/lv_draw_eve_letter.c \
../Core/Src/lvgl/draw/eve/lv_draw_eve_line.c \
../Core/Src/lvgl/draw/eve/lv_draw_eve_ram_g.c \
../Core/Src/lvgl/draw/eve/lv_draw_eve_triangle.c \
../Core/Src/lvgl/draw/eve/lv_eve.c 

OBJS += \
./Core/Src/lvgl/draw/eve/lv_draw_eve.o \
./Core/Src/lvgl/draw/eve/lv_draw_eve_arc.o \
./Core/Src/lvgl/draw/eve/lv_draw_eve_fill.o \
./Core/Src/lvgl/draw/eve/lv_draw_eve_image.o \
./Core/Src/lvgl/draw/eve/lv_draw_eve_letter.o \
./Core/Src/lvgl/draw/eve/lv_draw_eve_line.o \
./Core/Src/lvgl/draw/eve/lv_draw_eve_ram_g.o \
./Core/Src/lvgl/draw/eve/lv_draw_eve_triangle.o \
./Core/Src/lvgl/draw/eve/lv_eve.o 

C_DEPS += \
./Core/Src/lvgl/draw/eve/lv_draw_eve.d \
./Core/Src/lvgl/draw/eve/lv_draw_eve_arc.d \
./Core/Src/lvgl/draw/eve/lv_draw_eve_fill.d \
./Core/Src/lvgl/draw/eve/lv_draw_eve_image.d \
./Core/Src/lvgl/draw/eve/lv_draw_eve_letter.d \
./Core/Src/lvgl/draw/eve/lv_draw_eve_line.d \
./Core/Src/lvgl/draw/eve/lv_draw_eve_ram_g.d \
./Core/Src/lvgl/draw/eve/lv_draw_eve_triangle.d \
./Core/Src/lvgl/draw/eve/lv_eve.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/lvgl/draw/eve/%.o Core/Src/lvgl/draw/eve/%.su Core/Src/lvgl/draw/eve/%.cyclo: ../Core/Src/lvgl/draw/eve/%.c Core/Src/lvgl/draw/eve/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F405xx -c -I../FATFS/Target -I../FATFS/App -I../USB_HOST/App -I../USB_HOST/Target -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FatFs/src -I../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Host_Library/Class/HID/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -Oz -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-lvgl-2f-draw-2f-eve

clean-Core-2f-Src-2f-lvgl-2f-draw-2f-eve:
	-$(RM) ./Core/Src/lvgl/draw/eve/lv_draw_eve.cyclo ./Core/Src/lvgl/draw/eve/lv_draw_eve.d ./Core/Src/lvgl/draw/eve/lv_draw_eve.o ./Core/Src/lvgl/draw/eve/lv_draw_eve.su ./Core/Src/lvgl/draw/eve/lv_draw_eve_arc.cyclo ./Core/Src/lvgl/draw/eve/lv_draw_eve_arc.d ./Core/Src/lvgl/draw/eve/lv_draw_eve_arc.o ./Core/Src/lvgl/draw/eve/lv_draw_eve_arc.su ./Core/Src/lvgl/draw/eve/lv_draw_eve_fill.cyclo ./Core/Src/lvgl/draw/eve/lv_draw_eve_fill.d ./Core/Src/lvgl/draw/eve/lv_draw_eve_fill.o ./Core/Src/lvgl/draw/eve/lv_draw_eve_fill.su ./Core/Src/lvgl/draw/eve/lv_draw_eve_image.cyclo ./Core/Src/lvgl/draw/eve/lv_draw_eve_image.d ./Core/Src/lvgl/draw/eve/lv_draw_eve_image.o ./Core/Src/lvgl/draw/eve/lv_draw_eve_image.su ./Core/Src/lvgl/draw/eve/lv_draw_eve_letter.cyclo ./Core/Src/lvgl/draw/eve/lv_draw_eve_letter.d ./Core/Src/lvgl/draw/eve/lv_draw_eve_letter.o ./Core/Src/lvgl/draw/eve/lv_draw_eve_letter.su ./Core/Src/lvgl/draw/eve/lv_draw_eve_line.cyclo ./Core/Src/lvgl/draw/eve/lv_draw_eve_line.d ./Core/Src/lvgl/draw/eve/lv_draw_eve_line.o ./Core/Src/lvgl/draw/eve/lv_draw_eve_line.su ./Core/Src/lvgl/draw/eve/lv_draw_eve_ram_g.cyclo ./Core/Src/lvgl/draw/eve/lv_draw_eve_ram_g.d ./Core/Src/lvgl/draw/eve/lv_draw_eve_ram_g.o ./Core/Src/lvgl/draw/eve/lv_draw_eve_ram_g.su ./Core/Src/lvgl/draw/eve/lv_draw_eve_triangle.cyclo ./Core/Src/lvgl/draw/eve/lv_draw_eve_triangle.d ./Core/Src/lvgl/draw/eve/lv_draw_eve_triangle.o ./Core/Src/lvgl/draw/eve/lv_draw_eve_triangle.su ./Core/Src/lvgl/draw/eve/lv_eve.cyclo ./Core/Src/lvgl/draw/eve/lv_eve.d ./Core/Src/lvgl/draw/eve/lv_eve.o ./Core/Src/lvgl/draw/eve/lv_eve.su

.PHONY: clean-Core-2f-Src-2f-lvgl-2f-draw-2f-eve


################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/lvgl/widgets/property/lv_animimage_properties.c \
../Core/Src/lvgl/widgets/property/lv_arc_properties.c \
../Core/Src/lvgl/widgets/property/lv_bar_properties.c \
../Core/Src/lvgl/widgets/property/lv_buttonmatrix_properties.c \
../Core/Src/lvgl/widgets/property/lv_chart_properties.c \
../Core/Src/lvgl/widgets/property/lv_checkbox_properties.c \
../Core/Src/lvgl/widgets/property/lv_dropdown_properties.c \
../Core/Src/lvgl/widgets/property/lv_image_properties.c \
../Core/Src/lvgl/widgets/property/lv_keyboard_properties.c \
../Core/Src/lvgl/widgets/property/lv_label_properties.c \
../Core/Src/lvgl/widgets/property/lv_led_properties.c \
../Core/Src/lvgl/widgets/property/lv_line_properties.c \
../Core/Src/lvgl/widgets/property/lv_menu_properties.c \
../Core/Src/lvgl/widgets/property/lv_obj_properties.c \
../Core/Src/lvgl/widgets/property/lv_roller_properties.c \
../Core/Src/lvgl/widgets/property/lv_scale_properties.c \
../Core/Src/lvgl/widgets/property/lv_slider_properties.c \
../Core/Src/lvgl/widgets/property/lv_span_properties.c \
../Core/Src/lvgl/widgets/property/lv_spinbox_properties.c \
../Core/Src/lvgl/widgets/property/lv_spinner_properties.c \
../Core/Src/lvgl/widgets/property/lv_style_properties.c \
../Core/Src/lvgl/widgets/property/lv_switch_properties.c \
../Core/Src/lvgl/widgets/property/lv_table_properties.c \
../Core/Src/lvgl/widgets/property/lv_tabview_properties.c \
../Core/Src/lvgl/widgets/property/lv_textarea_properties.c 

OBJS += \
./Core/Src/lvgl/widgets/property/lv_animimage_properties.o \
./Core/Src/lvgl/widgets/property/lv_arc_properties.o \
./Core/Src/lvgl/widgets/property/lv_bar_properties.o \
./Core/Src/lvgl/widgets/property/lv_buttonmatrix_properties.o \
./Core/Src/lvgl/widgets/property/lv_chart_properties.o \
./Core/Src/lvgl/widgets/property/lv_checkbox_properties.o \
./Core/Src/lvgl/widgets/property/lv_dropdown_properties.o \
./Core/Src/lvgl/widgets/property/lv_image_properties.o \
./Core/Src/lvgl/widgets/property/lv_keyboard_properties.o \
./Core/Src/lvgl/widgets/property/lv_label_properties.o \
./Core/Src/lvgl/widgets/property/lv_led_properties.o \
./Core/Src/lvgl/widgets/property/lv_line_properties.o \
./Core/Src/lvgl/widgets/property/lv_menu_properties.o \
./Core/Src/lvgl/widgets/property/lv_obj_properties.o \
./Core/Src/lvgl/widgets/property/lv_roller_properties.o \
./Core/Src/lvgl/widgets/property/lv_scale_properties.o \
./Core/Src/lvgl/widgets/property/lv_slider_properties.o \
./Core/Src/lvgl/widgets/property/lv_span_properties.o \
./Core/Src/lvgl/widgets/property/lv_spinbox_properties.o \
./Core/Src/lvgl/widgets/property/lv_spinner_properties.o \
./Core/Src/lvgl/widgets/property/lv_style_properties.o \
./Core/Src/lvgl/widgets/property/lv_switch_properties.o \
./Core/Src/lvgl/widgets/property/lv_table_properties.o \
./Core/Src/lvgl/widgets/property/lv_tabview_properties.o \
./Core/Src/lvgl/widgets/property/lv_textarea_properties.o 

C_DEPS += \
./Core/Src/lvgl/widgets/property/lv_animimage_properties.d \
./Core/Src/lvgl/widgets/property/lv_arc_properties.d \
./Core/Src/lvgl/widgets/property/lv_bar_properties.d \
./Core/Src/lvgl/widgets/property/lv_buttonmatrix_properties.d \
./Core/Src/lvgl/widgets/property/lv_chart_properties.d \
./Core/Src/lvgl/widgets/property/lv_checkbox_properties.d \
./Core/Src/lvgl/widgets/property/lv_dropdown_properties.d \
./Core/Src/lvgl/widgets/property/lv_image_properties.d \
./Core/Src/lvgl/widgets/property/lv_keyboard_properties.d \
./Core/Src/lvgl/widgets/property/lv_label_properties.d \
./Core/Src/lvgl/widgets/property/lv_led_properties.d \
./Core/Src/lvgl/widgets/property/lv_line_properties.d \
./Core/Src/lvgl/widgets/property/lv_menu_properties.d \
./Core/Src/lvgl/widgets/property/lv_obj_properties.d \
./Core/Src/lvgl/widgets/property/lv_roller_properties.d \
./Core/Src/lvgl/widgets/property/lv_scale_properties.d \
./Core/Src/lvgl/widgets/property/lv_slider_properties.d \
./Core/Src/lvgl/widgets/property/lv_span_properties.d \
./Core/Src/lvgl/widgets/property/lv_spinbox_properties.d \
./Core/Src/lvgl/widgets/property/lv_spinner_properties.d \
./Core/Src/lvgl/widgets/property/lv_style_properties.d \
./Core/Src/lvgl/widgets/property/lv_switch_properties.d \
./Core/Src/lvgl/widgets/property/lv_table_properties.d \
./Core/Src/lvgl/widgets/property/lv_tabview_properties.d \
./Core/Src/lvgl/widgets/property/lv_textarea_properties.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/lvgl/widgets/property/%.o Core/Src/lvgl/widgets/property/%.su Core/Src/lvgl/widgets/property/%.cyclo: ../Core/Src/lvgl/widgets/property/%.c Core/Src/lvgl/widgets/property/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F405xx -c -I../FATFS/Target -I../FATFS/App -I../USB_HOST/App -I../USB_HOST/Target -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FatFs/src -I../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Host_Library/Class/HID/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -Oz -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-lvgl-2f-widgets-2f-property

clean-Core-2f-Src-2f-lvgl-2f-widgets-2f-property:
	-$(RM) ./Core/Src/lvgl/widgets/property/lv_animimage_properties.cyclo ./Core/Src/lvgl/widgets/property/lv_animimage_properties.d ./Core/Src/lvgl/widgets/property/lv_animimage_properties.o ./Core/Src/lvgl/widgets/property/lv_animimage_properties.su ./Core/Src/lvgl/widgets/property/lv_arc_properties.cyclo ./Core/Src/lvgl/widgets/property/lv_arc_properties.d ./Core/Src/lvgl/widgets/property/lv_arc_properties.o ./Core/Src/lvgl/widgets/property/lv_arc_properties.su ./Core/Src/lvgl/widgets/property/lv_bar_properties.cyclo ./Core/Src/lvgl/widgets/property/lv_bar_properties.d ./Core/Src/lvgl/widgets/property/lv_bar_properties.o ./Core/Src/lvgl/widgets/property/lv_bar_properties.su ./Core/Src/lvgl/widgets/property/lv_buttonmatrix_properties.cyclo ./Core/Src/lvgl/widgets/property/lv_buttonmatrix_properties.d ./Core/Src/lvgl/widgets/property/lv_buttonmatrix_properties.o ./Core/Src/lvgl/widgets/property/lv_buttonmatrix_properties.su ./Core/Src/lvgl/widgets/property/lv_chart_properties.cyclo ./Core/Src/lvgl/widgets/property/lv_chart_properties.d ./Core/Src/lvgl/widgets/property/lv_chart_properties.o ./Core/Src/lvgl/widgets/property/lv_chart_properties.su ./Core/Src/lvgl/widgets/property/lv_checkbox_properties.cyclo ./Core/Src/lvgl/widgets/property/lv_checkbox_properties.d ./Core/Src/lvgl/widgets/property/lv_checkbox_properties.o ./Core/Src/lvgl/widgets/property/lv_checkbox_properties.su ./Core/Src/lvgl/widgets/property/lv_dropdown_properties.cyclo ./Core/Src/lvgl/widgets/property/lv_dropdown_properties.d ./Core/Src/lvgl/widgets/property/lv_dropdown_properties.o ./Core/Src/lvgl/widgets/property/lv_dropdown_properties.su ./Core/Src/lvgl/widgets/property/lv_image_properties.cyclo ./Core/Src/lvgl/widgets/property/lv_image_properties.d ./Core/Src/lvgl/widgets/property/lv_image_properties.o ./Core/Src/lvgl/widgets/property/lv_image_properties.su ./Core/Src/lvgl/widgets/property/lv_keyboard_properties.cyclo ./Core/Src/lvgl/widgets/property/lv_keyboard_properties.d ./Core/Src/lvgl/widgets/property/lv_keyboard_properties.o ./Core/Src/lvgl/widgets/property/lv_keyboard_properties.su ./Core/Src/lvgl/widgets/property/lv_label_properties.cyclo ./Core/Src/lvgl/widgets/property/lv_label_properties.d ./Core/Src/lvgl/widgets/property/lv_label_properties.o ./Core/Src/lvgl/widgets/property/lv_label_properties.su ./Core/Src/lvgl/widgets/property/lv_led_properties.cyclo ./Core/Src/lvgl/widgets/property/lv_led_properties.d ./Core/Src/lvgl/widgets/property/lv_led_properties.o ./Core/Src/lvgl/widgets/property/lv_led_properties.su ./Core/Src/lvgl/widgets/property/lv_line_properties.cyclo ./Core/Src/lvgl/widgets/property/lv_line_properties.d ./Core/Src/lvgl/widgets/property/lv_line_properties.o ./Core/Src/lvgl/widgets/property/lv_line_properties.su ./Core/Src/lvgl/widgets/property/lv_menu_properties.cyclo ./Core/Src/lvgl/widgets/property/lv_menu_properties.d ./Core/Src/lvgl/widgets/property/lv_menu_properties.o ./Core/Src/lvgl/widgets/property/lv_menu_properties.su ./Core/Src/lvgl/widgets/property/lv_obj_properties.cyclo ./Core/Src/lvgl/widgets/property/lv_obj_properties.d ./Core/Src/lvgl/widgets/property/lv_obj_properties.o ./Core/Src/lvgl/widgets/property/lv_obj_properties.su ./Core/Src/lvgl/widgets/property/lv_roller_properties.cyclo ./Core/Src/lvgl/widgets/property/lv_roller_properties.d ./Core/Src/lvgl/widgets/property/lv_roller_properties.o ./Core/Src/lvgl/widgets/property/lv_roller_properties.su ./Core/Src/lvgl/widgets/property/lv_scale_properties.cyclo ./Core/Src/lvgl/widgets/property/lv_scale_properties.d ./Core/Src/lvgl/widgets/property/lv_scale_properties.o ./Core/Src/lvgl/widgets/property/lv_scale_properties.su ./Core/Src/lvgl/widgets/property/lv_slider_properties.cyclo ./Core/Src/lvgl/widgets/property/lv_slider_properties.d ./Core/Src/lvgl/widgets/property/lv_slider_properties.o ./Core/Src/lvgl/widgets/property/lv_slider_properties.su ./Core/Src/lvgl/widgets/property/lv_span_properties.cyclo ./Core/Src/lvgl/widgets/property/lv_span_properties.d ./Core/Src/lvgl/widgets/property/lv_span_properties.o ./Core/Src/lvgl/widgets/property/lv_span_properties.su ./Core/Src/lvgl/widgets/property/lv_spinbox_properties.cyclo ./Core/Src/lvgl/widgets/property/lv_spinbox_properties.d ./Core/Src/lvgl/widgets/property/lv_spinbox_properties.o ./Core/Src/lvgl/widgets/property/lv_spinbox_properties.su ./Core/Src/lvgl/widgets/property/lv_spinner_properties.cyclo ./Core/Src/lvgl/widgets/property/lv_spinner_properties.d ./Core/Src/lvgl/widgets/property/lv_spinner_properties.o ./Core/Src/lvgl/widgets/property/lv_spinner_properties.su ./Core/Src/lvgl/widgets/property/lv_style_properties.cyclo ./Core/Src/lvgl/widgets/property/lv_style_properties.d ./Core/Src/lvgl/widgets/property/lv_style_properties.o ./Core/Src/lvgl/widgets/property/lv_style_properties.su ./Core/Src/lvgl/widgets/property/lv_switch_properties.cyclo ./Core/Src/lvgl/widgets/property/lv_switch_properties.d ./Core/Src/lvgl/widgets/property/lv_switch_properties.o ./Core/Src/lvgl/widgets/property/lv_switch_properties.su ./Core/Src/lvgl/widgets/property/lv_table_properties.cyclo ./Core/Src/lvgl/widgets/property/lv_table_properties.d ./Core/Src/lvgl/widgets/property/lv_table_properties.o ./Core/Src/lvgl/widgets/property/lv_table_properties.su ./Core/Src/lvgl/widgets/property/lv_tabview_properties.cyclo ./Core/Src/lvgl/widgets/property/lv_tabview_properties.d ./Core/Src/lvgl/widgets/property/lv_tabview_properties.o ./Core/Src/lvgl/widgets/property/lv_tabview_properties.su ./Core/Src/lvgl/widgets/property/lv_textarea_properties.cyclo ./Core/Src/lvgl/widgets/property/lv_textarea_properties.d ./Core/Src/lvgl/widgets/property/lv_textarea_properties.o ./Core/Src/lvgl/widgets/property/lv_textarea_properties.su

.PHONY: clean-Core-2f-Src-2f-lvgl-2f-widgets-2f-property


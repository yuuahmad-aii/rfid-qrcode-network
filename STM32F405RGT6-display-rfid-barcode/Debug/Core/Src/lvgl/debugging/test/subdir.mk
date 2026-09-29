################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/lvgl/debugging/test/lv_test_display.c \
../Core/Src/lvgl/debugging/test/lv_test_fs.c \
../Core/Src/lvgl/debugging/test/lv_test_helpers.c \
../Core/Src/lvgl/debugging/test/lv_test_indev.c \
../Core/Src/lvgl/debugging/test/lv_test_indev_gesture.c \
../Core/Src/lvgl/debugging/test/lv_test_screenshot_compare.c 

OBJS += \
./Core/Src/lvgl/debugging/test/lv_test_display.o \
./Core/Src/lvgl/debugging/test/lv_test_fs.o \
./Core/Src/lvgl/debugging/test/lv_test_helpers.o \
./Core/Src/lvgl/debugging/test/lv_test_indev.o \
./Core/Src/lvgl/debugging/test/lv_test_indev_gesture.o \
./Core/Src/lvgl/debugging/test/lv_test_screenshot_compare.o 

C_DEPS += \
./Core/Src/lvgl/debugging/test/lv_test_display.d \
./Core/Src/lvgl/debugging/test/lv_test_fs.d \
./Core/Src/lvgl/debugging/test/lv_test_helpers.d \
./Core/Src/lvgl/debugging/test/lv_test_indev.d \
./Core/Src/lvgl/debugging/test/lv_test_indev_gesture.d \
./Core/Src/lvgl/debugging/test/lv_test_screenshot_compare.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/lvgl/debugging/test/%.o Core/Src/lvgl/debugging/test/%.su Core/Src/lvgl/debugging/test/%.cyclo: ../Core/Src/lvgl/debugging/test/%.c Core/Src/lvgl/debugging/test/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F405xx -c -I../FATFS/Target -I../FATFS/App -I../USB_HOST/App -I../USB_HOST/Target -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FatFs/src -I../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Host_Library/Class/HID/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -Oz -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-lvgl-2f-debugging-2f-test

clean-Core-2f-Src-2f-lvgl-2f-debugging-2f-test:
	-$(RM) ./Core/Src/lvgl/debugging/test/lv_test_display.cyclo ./Core/Src/lvgl/debugging/test/lv_test_display.d ./Core/Src/lvgl/debugging/test/lv_test_display.o ./Core/Src/lvgl/debugging/test/lv_test_display.su ./Core/Src/lvgl/debugging/test/lv_test_fs.cyclo ./Core/Src/lvgl/debugging/test/lv_test_fs.d ./Core/Src/lvgl/debugging/test/lv_test_fs.o ./Core/Src/lvgl/debugging/test/lv_test_fs.su ./Core/Src/lvgl/debugging/test/lv_test_helpers.cyclo ./Core/Src/lvgl/debugging/test/lv_test_helpers.d ./Core/Src/lvgl/debugging/test/lv_test_helpers.o ./Core/Src/lvgl/debugging/test/lv_test_helpers.su ./Core/Src/lvgl/debugging/test/lv_test_indev.cyclo ./Core/Src/lvgl/debugging/test/lv_test_indev.d ./Core/Src/lvgl/debugging/test/lv_test_indev.o ./Core/Src/lvgl/debugging/test/lv_test_indev.su ./Core/Src/lvgl/debugging/test/lv_test_indev_gesture.cyclo ./Core/Src/lvgl/debugging/test/lv_test_indev_gesture.d ./Core/Src/lvgl/debugging/test/lv_test_indev_gesture.o ./Core/Src/lvgl/debugging/test/lv_test_indev_gesture.su ./Core/Src/lvgl/debugging/test/lv_test_screenshot_compare.cyclo ./Core/Src/lvgl/debugging/test/lv_test_screenshot_compare.d ./Core/Src/lvgl/debugging/test/lv_test_screenshot_compare.o ./Core/Src/lvgl/debugging/test/lv_test_screenshot_compare.su

.PHONY: clean-Core-2f-Src-2f-lvgl-2f-debugging-2f-test


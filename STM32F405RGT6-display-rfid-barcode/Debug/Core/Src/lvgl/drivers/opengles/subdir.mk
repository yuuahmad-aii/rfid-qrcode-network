################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/lvgl/drivers/opengles/lv_opengles_debug.c \
../Core/Src/lvgl/drivers/opengles/lv_opengles_driver.c \
../Core/Src/lvgl/drivers/opengles/lv_opengles_egl.c \
../Core/Src/lvgl/drivers/opengles/lv_opengles_glfw.c \
../Core/Src/lvgl/drivers/opengles/lv_opengles_texture.c 

OBJS += \
./Core/Src/lvgl/drivers/opengles/lv_opengles_debug.o \
./Core/Src/lvgl/drivers/opengles/lv_opengles_driver.o \
./Core/Src/lvgl/drivers/opengles/lv_opengles_egl.o \
./Core/Src/lvgl/drivers/opengles/lv_opengles_glfw.o \
./Core/Src/lvgl/drivers/opengles/lv_opengles_texture.o 

C_DEPS += \
./Core/Src/lvgl/drivers/opengles/lv_opengles_debug.d \
./Core/Src/lvgl/drivers/opengles/lv_opengles_driver.d \
./Core/Src/lvgl/drivers/opengles/lv_opengles_egl.d \
./Core/Src/lvgl/drivers/opengles/lv_opengles_glfw.d \
./Core/Src/lvgl/drivers/opengles/lv_opengles_texture.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/lvgl/drivers/opengles/%.o Core/Src/lvgl/drivers/opengles/%.su Core/Src/lvgl/drivers/opengles/%.cyclo: ../Core/Src/lvgl/drivers/opengles/%.c Core/Src/lvgl/drivers/opengles/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F405xx -c -I../FATFS/Target -I../FATFS/App -I../USB_HOST/App -I../USB_HOST/Target -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FatFs/src -I../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Host_Library/Class/HID/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -Oz -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-lvgl-2f-drivers-2f-opengles

clean-Core-2f-Src-2f-lvgl-2f-drivers-2f-opengles:
	-$(RM) ./Core/Src/lvgl/drivers/opengles/lv_opengles_debug.cyclo ./Core/Src/lvgl/drivers/opengles/lv_opengles_debug.d ./Core/Src/lvgl/drivers/opengles/lv_opengles_debug.o ./Core/Src/lvgl/drivers/opengles/lv_opengles_debug.su ./Core/Src/lvgl/drivers/opengles/lv_opengles_driver.cyclo ./Core/Src/lvgl/drivers/opengles/lv_opengles_driver.d ./Core/Src/lvgl/drivers/opengles/lv_opengles_driver.o ./Core/Src/lvgl/drivers/opengles/lv_opengles_driver.su ./Core/Src/lvgl/drivers/opengles/lv_opengles_egl.cyclo ./Core/Src/lvgl/drivers/opengles/lv_opengles_egl.d ./Core/Src/lvgl/drivers/opengles/lv_opengles_egl.o ./Core/Src/lvgl/drivers/opengles/lv_opengles_egl.su ./Core/Src/lvgl/drivers/opengles/lv_opengles_glfw.cyclo ./Core/Src/lvgl/drivers/opengles/lv_opengles_glfw.d ./Core/Src/lvgl/drivers/opengles/lv_opengles_glfw.o ./Core/Src/lvgl/drivers/opengles/lv_opengles_glfw.su ./Core/Src/lvgl/drivers/opengles/lv_opengles_texture.cyclo ./Core/Src/lvgl/drivers/opengles/lv_opengles_texture.d ./Core/Src/lvgl/drivers/opengles/lv_opengles_texture.o ./Core/Src/lvgl/drivers/opengles/lv_opengles_texture.su

.PHONY: clean-Core-2f-Src-2f-lvgl-2f-drivers-2f-opengles


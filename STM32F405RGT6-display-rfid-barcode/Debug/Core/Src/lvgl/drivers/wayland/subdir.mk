################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/lvgl/drivers/wayland/lv_wayland.c \
../Core/Src/lvgl/drivers/wayland/lv_wl_egl_backend.c \
../Core/Src/lvgl/drivers/wayland/lv_wl_g2d_backend.c \
../Core/Src/lvgl/drivers/wayland/lv_wl_keyboard.c \
../Core/Src/lvgl/drivers/wayland/lv_wl_pointer.c \
../Core/Src/lvgl/drivers/wayland/lv_wl_seat.c \
../Core/Src/lvgl/drivers/wayland/lv_wl_shm_backend.c \
../Core/Src/lvgl/drivers/wayland/lv_wl_touch.c \
../Core/Src/lvgl/drivers/wayland/lv_wl_window.c \
../Core/Src/lvgl/drivers/wayland/lv_wl_xdg_shell.c 

OBJS += \
./Core/Src/lvgl/drivers/wayland/lv_wayland.o \
./Core/Src/lvgl/drivers/wayland/lv_wl_egl_backend.o \
./Core/Src/lvgl/drivers/wayland/lv_wl_g2d_backend.o \
./Core/Src/lvgl/drivers/wayland/lv_wl_keyboard.o \
./Core/Src/lvgl/drivers/wayland/lv_wl_pointer.o \
./Core/Src/lvgl/drivers/wayland/lv_wl_seat.o \
./Core/Src/lvgl/drivers/wayland/lv_wl_shm_backend.o \
./Core/Src/lvgl/drivers/wayland/lv_wl_touch.o \
./Core/Src/lvgl/drivers/wayland/lv_wl_window.o \
./Core/Src/lvgl/drivers/wayland/lv_wl_xdg_shell.o 

C_DEPS += \
./Core/Src/lvgl/drivers/wayland/lv_wayland.d \
./Core/Src/lvgl/drivers/wayland/lv_wl_egl_backend.d \
./Core/Src/lvgl/drivers/wayland/lv_wl_g2d_backend.d \
./Core/Src/lvgl/drivers/wayland/lv_wl_keyboard.d \
./Core/Src/lvgl/drivers/wayland/lv_wl_pointer.d \
./Core/Src/lvgl/drivers/wayland/lv_wl_seat.d \
./Core/Src/lvgl/drivers/wayland/lv_wl_shm_backend.d \
./Core/Src/lvgl/drivers/wayland/lv_wl_touch.d \
./Core/Src/lvgl/drivers/wayland/lv_wl_window.d \
./Core/Src/lvgl/drivers/wayland/lv_wl_xdg_shell.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/lvgl/drivers/wayland/%.o Core/Src/lvgl/drivers/wayland/%.su Core/Src/lvgl/drivers/wayland/%.cyclo: ../Core/Src/lvgl/drivers/wayland/%.c Core/Src/lvgl/drivers/wayland/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F405xx -c -I../FATFS/Target -I../FATFS/App -I../USB_HOST/App -I../USB_HOST/Target -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FatFs/src -I../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Host_Library/Class/HID/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -Oz -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-lvgl-2f-drivers-2f-wayland

clean-Core-2f-Src-2f-lvgl-2f-drivers-2f-wayland:
	-$(RM) ./Core/Src/lvgl/drivers/wayland/lv_wayland.cyclo ./Core/Src/lvgl/drivers/wayland/lv_wayland.d ./Core/Src/lvgl/drivers/wayland/lv_wayland.o ./Core/Src/lvgl/drivers/wayland/lv_wayland.su ./Core/Src/lvgl/drivers/wayland/lv_wl_egl_backend.cyclo ./Core/Src/lvgl/drivers/wayland/lv_wl_egl_backend.d ./Core/Src/lvgl/drivers/wayland/lv_wl_egl_backend.o ./Core/Src/lvgl/drivers/wayland/lv_wl_egl_backend.su ./Core/Src/lvgl/drivers/wayland/lv_wl_g2d_backend.cyclo ./Core/Src/lvgl/drivers/wayland/lv_wl_g2d_backend.d ./Core/Src/lvgl/drivers/wayland/lv_wl_g2d_backend.o ./Core/Src/lvgl/drivers/wayland/lv_wl_g2d_backend.su ./Core/Src/lvgl/drivers/wayland/lv_wl_keyboard.cyclo ./Core/Src/lvgl/drivers/wayland/lv_wl_keyboard.d ./Core/Src/lvgl/drivers/wayland/lv_wl_keyboard.o ./Core/Src/lvgl/drivers/wayland/lv_wl_keyboard.su ./Core/Src/lvgl/drivers/wayland/lv_wl_pointer.cyclo ./Core/Src/lvgl/drivers/wayland/lv_wl_pointer.d ./Core/Src/lvgl/drivers/wayland/lv_wl_pointer.o ./Core/Src/lvgl/drivers/wayland/lv_wl_pointer.su ./Core/Src/lvgl/drivers/wayland/lv_wl_seat.cyclo ./Core/Src/lvgl/drivers/wayland/lv_wl_seat.d ./Core/Src/lvgl/drivers/wayland/lv_wl_seat.o ./Core/Src/lvgl/drivers/wayland/lv_wl_seat.su ./Core/Src/lvgl/drivers/wayland/lv_wl_shm_backend.cyclo ./Core/Src/lvgl/drivers/wayland/lv_wl_shm_backend.d ./Core/Src/lvgl/drivers/wayland/lv_wl_shm_backend.o ./Core/Src/lvgl/drivers/wayland/lv_wl_shm_backend.su ./Core/Src/lvgl/drivers/wayland/lv_wl_touch.cyclo ./Core/Src/lvgl/drivers/wayland/lv_wl_touch.d ./Core/Src/lvgl/drivers/wayland/lv_wl_touch.o ./Core/Src/lvgl/drivers/wayland/lv_wl_touch.su ./Core/Src/lvgl/drivers/wayland/lv_wl_window.cyclo ./Core/Src/lvgl/drivers/wayland/lv_wl_window.d ./Core/Src/lvgl/drivers/wayland/lv_wl_window.o ./Core/Src/lvgl/drivers/wayland/lv_wl_window.su ./Core/Src/lvgl/drivers/wayland/lv_wl_xdg_shell.cyclo ./Core/Src/lvgl/drivers/wayland/lv_wl_xdg_shell.d ./Core/Src/lvgl/drivers/wayland/lv_wl_xdg_shell.o ./Core/Src/lvgl/drivers/wayland/lv_wl_xdg_shell.su

.PHONY: clean-Core-2f-Src-2f-lvgl-2f-drivers-2f-wayland


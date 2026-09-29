################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/lvgl/osal/lv_cmsis_rtos2.c \
../Core/Src/lvgl/osal/lv_freertos.c \
../Core/Src/lvgl/osal/lv_linux.c \
../Core/Src/lvgl/osal/lv_mqx.c \
../Core/Src/lvgl/osal/lv_os.c \
../Core/Src/lvgl/osal/lv_os_none.c \
../Core/Src/lvgl/osal/lv_pthread.c \
../Core/Src/lvgl/osal/lv_rtthread.c \
../Core/Src/lvgl/osal/lv_sdl2.c \
../Core/Src/lvgl/osal/lv_windows.c 

OBJS += \
./Core/Src/lvgl/osal/lv_cmsis_rtos2.o \
./Core/Src/lvgl/osal/lv_freertos.o \
./Core/Src/lvgl/osal/lv_linux.o \
./Core/Src/lvgl/osal/lv_mqx.o \
./Core/Src/lvgl/osal/lv_os.o \
./Core/Src/lvgl/osal/lv_os_none.o \
./Core/Src/lvgl/osal/lv_pthread.o \
./Core/Src/lvgl/osal/lv_rtthread.o \
./Core/Src/lvgl/osal/lv_sdl2.o \
./Core/Src/lvgl/osal/lv_windows.o 

C_DEPS += \
./Core/Src/lvgl/osal/lv_cmsis_rtos2.d \
./Core/Src/lvgl/osal/lv_freertos.d \
./Core/Src/lvgl/osal/lv_linux.d \
./Core/Src/lvgl/osal/lv_mqx.d \
./Core/Src/lvgl/osal/lv_os.d \
./Core/Src/lvgl/osal/lv_os_none.d \
./Core/Src/lvgl/osal/lv_pthread.d \
./Core/Src/lvgl/osal/lv_rtthread.d \
./Core/Src/lvgl/osal/lv_sdl2.d \
./Core/Src/lvgl/osal/lv_windows.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/lvgl/osal/%.o Core/Src/lvgl/osal/%.su Core/Src/lvgl/osal/%.cyclo: ../Core/Src/lvgl/osal/%.c Core/Src/lvgl/osal/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F405xx -c -I../FATFS/Target -I../FATFS/App -I../USB_HOST/App -I../USB_HOST/Target -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FatFs/src -I../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Host_Library/Class/HID/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -Oz -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-lvgl-2f-osal

clean-Core-2f-Src-2f-lvgl-2f-osal:
	-$(RM) ./Core/Src/lvgl/osal/lv_cmsis_rtos2.cyclo ./Core/Src/lvgl/osal/lv_cmsis_rtos2.d ./Core/Src/lvgl/osal/lv_cmsis_rtos2.o ./Core/Src/lvgl/osal/lv_cmsis_rtos2.su ./Core/Src/lvgl/osal/lv_freertos.cyclo ./Core/Src/lvgl/osal/lv_freertos.d ./Core/Src/lvgl/osal/lv_freertos.o ./Core/Src/lvgl/osal/lv_freertos.su ./Core/Src/lvgl/osal/lv_linux.cyclo ./Core/Src/lvgl/osal/lv_linux.d ./Core/Src/lvgl/osal/lv_linux.o ./Core/Src/lvgl/osal/lv_linux.su ./Core/Src/lvgl/osal/lv_mqx.cyclo ./Core/Src/lvgl/osal/lv_mqx.d ./Core/Src/lvgl/osal/lv_mqx.o ./Core/Src/lvgl/osal/lv_mqx.su ./Core/Src/lvgl/osal/lv_os.cyclo ./Core/Src/lvgl/osal/lv_os.d ./Core/Src/lvgl/osal/lv_os.o ./Core/Src/lvgl/osal/lv_os.su ./Core/Src/lvgl/osal/lv_os_none.cyclo ./Core/Src/lvgl/osal/lv_os_none.d ./Core/Src/lvgl/osal/lv_os_none.o ./Core/Src/lvgl/osal/lv_os_none.su ./Core/Src/lvgl/osal/lv_pthread.cyclo ./Core/Src/lvgl/osal/lv_pthread.d ./Core/Src/lvgl/osal/lv_pthread.o ./Core/Src/lvgl/osal/lv_pthread.su ./Core/Src/lvgl/osal/lv_rtthread.cyclo ./Core/Src/lvgl/osal/lv_rtthread.d ./Core/Src/lvgl/osal/lv_rtthread.o ./Core/Src/lvgl/osal/lv_rtthread.su ./Core/Src/lvgl/osal/lv_sdl2.cyclo ./Core/Src/lvgl/osal/lv_sdl2.d ./Core/Src/lvgl/osal/lv_sdl2.o ./Core/Src/lvgl/osal/lv_sdl2.su ./Core/Src/lvgl/osal/lv_windows.cyclo ./Core/Src/lvgl/osal/lv_windows.d ./Core/Src/lvgl/osal/lv_windows.o ./Core/Src/lvgl/osal/lv_windows.su

.PHONY: clean-Core-2f-Src-2f-lvgl-2f-osal


################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/lvgl/misc/cache/lv_cache.c \
../Core/Src/lvgl/misc/cache/lv_cache_entry.c 

OBJS += \
./Core/Src/lvgl/misc/cache/lv_cache.o \
./Core/Src/lvgl/misc/cache/lv_cache_entry.o 

C_DEPS += \
./Core/Src/lvgl/misc/cache/lv_cache.d \
./Core/Src/lvgl/misc/cache/lv_cache_entry.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/lvgl/misc/cache/%.o Core/Src/lvgl/misc/cache/%.su Core/Src/lvgl/misc/cache/%.cyclo: ../Core/Src/lvgl/misc/cache/%.c Core/Src/lvgl/misc/cache/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F405xx -c -I../FATFS/Target -I../FATFS/App -I../USB_HOST/App -I../USB_HOST/Target -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FatFs/src -I../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Host_Library/Class/HID/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -Oz -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-lvgl-2f-misc-2f-cache

clean-Core-2f-Src-2f-lvgl-2f-misc-2f-cache:
	-$(RM) ./Core/Src/lvgl/misc/cache/lv_cache.cyclo ./Core/Src/lvgl/misc/cache/lv_cache.d ./Core/Src/lvgl/misc/cache/lv_cache.o ./Core/Src/lvgl/misc/cache/lv_cache.su ./Core/Src/lvgl/misc/cache/lv_cache_entry.cyclo ./Core/Src/lvgl/misc/cache/lv_cache_entry.d ./Core/Src/lvgl/misc/cache/lv_cache_entry.o ./Core/Src/lvgl/misc/cache/lv_cache_entry.su

.PHONY: clean-Core-2f-Src-2f-lvgl-2f-misc-2f-cache


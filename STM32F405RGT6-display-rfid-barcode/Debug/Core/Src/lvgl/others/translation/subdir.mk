################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/lvgl/others/translation/lv_translation.c 

OBJS += \
./Core/Src/lvgl/others/translation/lv_translation.o 

C_DEPS += \
./Core/Src/lvgl/others/translation/lv_translation.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/lvgl/others/translation/%.o Core/Src/lvgl/others/translation/%.su Core/Src/lvgl/others/translation/%.cyclo: ../Core/Src/lvgl/others/translation/%.c Core/Src/lvgl/others/translation/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F405xx -c -I../FATFS/Target -I../FATFS/App -I../USB_HOST/App -I../USB_HOST/Target -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FatFs/src -I../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Host_Library/Class/HID/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -Oz -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-lvgl-2f-others-2f-translation

clean-Core-2f-Src-2f-lvgl-2f-others-2f-translation:
	-$(RM) ./Core/Src/lvgl/others/translation/lv_translation.cyclo ./Core/Src/lvgl/others/translation/lv_translation.d ./Core/Src/lvgl/others/translation/lv_translation.o ./Core/Src/lvgl/others/translation/lv_translation.su

.PHONY: clean-Core-2f-Src-2f-lvgl-2f-others-2f-translation


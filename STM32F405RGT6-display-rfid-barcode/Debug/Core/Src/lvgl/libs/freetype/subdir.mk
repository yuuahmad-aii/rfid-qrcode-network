################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/lvgl/libs/freetype/lv_freetype.c \
../Core/Src/lvgl/libs/freetype/lv_freetype_glyph.c \
../Core/Src/lvgl/libs/freetype/lv_freetype_image.c \
../Core/Src/lvgl/libs/freetype/lv_freetype_outline.c \
../Core/Src/lvgl/libs/freetype/lv_ftsystem.c 

OBJS += \
./Core/Src/lvgl/libs/freetype/lv_freetype.o \
./Core/Src/lvgl/libs/freetype/lv_freetype_glyph.o \
./Core/Src/lvgl/libs/freetype/lv_freetype_image.o \
./Core/Src/lvgl/libs/freetype/lv_freetype_outline.o \
./Core/Src/lvgl/libs/freetype/lv_ftsystem.o 

C_DEPS += \
./Core/Src/lvgl/libs/freetype/lv_freetype.d \
./Core/Src/lvgl/libs/freetype/lv_freetype_glyph.d \
./Core/Src/lvgl/libs/freetype/lv_freetype_image.d \
./Core/Src/lvgl/libs/freetype/lv_freetype_outline.d \
./Core/Src/lvgl/libs/freetype/lv_ftsystem.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/lvgl/libs/freetype/%.o Core/Src/lvgl/libs/freetype/%.su Core/Src/lvgl/libs/freetype/%.cyclo: ../Core/Src/lvgl/libs/freetype/%.c Core/Src/lvgl/libs/freetype/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F405xx -c -I../FATFS/Target -I../FATFS/App -I../USB_HOST/App -I../USB_HOST/Target -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FatFs/src -I../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Host_Library/Class/HID/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -Oz -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-lvgl-2f-libs-2f-freetype

clean-Core-2f-Src-2f-lvgl-2f-libs-2f-freetype:
	-$(RM) ./Core/Src/lvgl/libs/freetype/lv_freetype.cyclo ./Core/Src/lvgl/libs/freetype/lv_freetype.d ./Core/Src/lvgl/libs/freetype/lv_freetype.o ./Core/Src/lvgl/libs/freetype/lv_freetype.su ./Core/Src/lvgl/libs/freetype/lv_freetype_glyph.cyclo ./Core/Src/lvgl/libs/freetype/lv_freetype_glyph.d ./Core/Src/lvgl/libs/freetype/lv_freetype_glyph.o ./Core/Src/lvgl/libs/freetype/lv_freetype_glyph.su ./Core/Src/lvgl/libs/freetype/lv_freetype_image.cyclo ./Core/Src/lvgl/libs/freetype/lv_freetype_image.d ./Core/Src/lvgl/libs/freetype/lv_freetype_image.o ./Core/Src/lvgl/libs/freetype/lv_freetype_image.su ./Core/Src/lvgl/libs/freetype/lv_freetype_outline.cyclo ./Core/Src/lvgl/libs/freetype/lv_freetype_outline.d ./Core/Src/lvgl/libs/freetype/lv_freetype_outline.o ./Core/Src/lvgl/libs/freetype/lv_freetype_outline.su ./Core/Src/lvgl/libs/freetype/lv_ftsystem.cyclo ./Core/Src/lvgl/libs/freetype/lv_ftsystem.d ./Core/Src/lvgl/libs/freetype/lv_ftsystem.o ./Core/Src/lvgl/libs/freetype/lv_ftsystem.su

.PHONY: clean-Core-2f-Src-2f-lvgl-2f-libs-2f-freetype


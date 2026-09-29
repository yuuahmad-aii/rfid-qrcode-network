################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/eez/images.c \
../Core/Src/eez/screens.c \
../Core/Src/eez/styles.c \
../Core/Src/eez/ui.c \
../Core/Src/eez/ui_font_fa13.c \
../Core/Src/eez/ui_font_fa14.c \
../Core/Src/eez/ui_font_fa16.c \
../Core/Src/eez/ui_font_fa18.c \
../Core/Src/eez/ui_font_fa20.c \
../Core/Src/eez/ui_font_fa24.c \
../Core/Src/eez/ui_font_rl104.c \
../Core/Src/eez/ui_font_rl26.c \
../Core/Src/eez/ui_font_rm10.c \
../Core/Src/eez/ui_font_rm11.c \
../Core/Src/eez/ui_font_rm13.c \
../Core/Src/eez/ui_font_rm14.c \
../Core/Src/eez/ui_font_rm15.c \
../Core/Src/eez/ui_font_rm26.c \
../Core/Src/eez/ui_font_rm9.c \
../Core/Src/eez/ui_font_rr11.c \
../Core/Src/eez/ui_font_rr13.c \
../Core/Src/eez/ui_font_rr14.c \
../Core/Src/eez/ui_font_rr15.c \
../Core/Src/eez/ui_font_rr20.c \
../Core/Src/eez/ui_font_rr22.c \
../Core/Src/eez/ui_image_tc_fireside.c 

OBJS += \
./Core/Src/eez/images.o \
./Core/Src/eez/screens.o \
./Core/Src/eez/styles.o \
./Core/Src/eez/ui.o \
./Core/Src/eez/ui_font_fa13.o \
./Core/Src/eez/ui_font_fa14.o \
./Core/Src/eez/ui_font_fa16.o \
./Core/Src/eez/ui_font_fa18.o \
./Core/Src/eez/ui_font_fa20.o \
./Core/Src/eez/ui_font_fa24.o \
./Core/Src/eez/ui_font_rl104.o \
./Core/Src/eez/ui_font_rl26.o \
./Core/Src/eez/ui_font_rm10.o \
./Core/Src/eez/ui_font_rm11.o \
./Core/Src/eez/ui_font_rm13.o \
./Core/Src/eez/ui_font_rm14.o \
./Core/Src/eez/ui_font_rm15.o \
./Core/Src/eez/ui_font_rm26.o \
./Core/Src/eez/ui_font_rm9.o \
./Core/Src/eez/ui_font_rr11.o \
./Core/Src/eez/ui_font_rr13.o \
./Core/Src/eez/ui_font_rr14.o \
./Core/Src/eez/ui_font_rr15.o \
./Core/Src/eez/ui_font_rr20.o \
./Core/Src/eez/ui_font_rr22.o \
./Core/Src/eez/ui_image_tc_fireside.o 

C_DEPS += \
./Core/Src/eez/images.d \
./Core/Src/eez/screens.d \
./Core/Src/eez/styles.d \
./Core/Src/eez/ui.d \
./Core/Src/eez/ui_font_fa13.d \
./Core/Src/eez/ui_font_fa14.d \
./Core/Src/eez/ui_font_fa16.d \
./Core/Src/eez/ui_font_fa18.d \
./Core/Src/eez/ui_font_fa20.d \
./Core/Src/eez/ui_font_fa24.d \
./Core/Src/eez/ui_font_rl104.d \
./Core/Src/eez/ui_font_rl26.d \
./Core/Src/eez/ui_font_rm10.d \
./Core/Src/eez/ui_font_rm11.d \
./Core/Src/eez/ui_font_rm13.d \
./Core/Src/eez/ui_font_rm14.d \
./Core/Src/eez/ui_font_rm15.d \
./Core/Src/eez/ui_font_rm26.d \
./Core/Src/eez/ui_font_rm9.d \
./Core/Src/eez/ui_font_rr11.d \
./Core/Src/eez/ui_font_rr13.d \
./Core/Src/eez/ui_font_rr14.d \
./Core/Src/eez/ui_font_rr15.d \
./Core/Src/eez/ui_font_rr20.d \
./Core/Src/eez/ui_font_rr22.d \
./Core/Src/eez/ui_image_tc_fireside.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/eez/%.o Core/Src/eez/%.su Core/Src/eez/%.cyclo: ../Core/Src/eez/%.c Core/Src/eez/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F405xx -c -I../FATFS/Target -I../FATFS/App -I../USB_HOST/App -I../USB_HOST/Target -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FatFs/src -I../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Host_Library/Class/HID/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -Oz -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-eez

clean-Core-2f-Src-2f-eez:
	-$(RM) ./Core/Src/eez/images.cyclo ./Core/Src/eez/images.d ./Core/Src/eez/images.o ./Core/Src/eez/images.su ./Core/Src/eez/screens.cyclo ./Core/Src/eez/screens.d ./Core/Src/eez/screens.o ./Core/Src/eez/screens.su ./Core/Src/eez/styles.cyclo ./Core/Src/eez/styles.d ./Core/Src/eez/styles.o ./Core/Src/eez/styles.su ./Core/Src/eez/ui.cyclo ./Core/Src/eez/ui.d ./Core/Src/eez/ui.o ./Core/Src/eez/ui.su ./Core/Src/eez/ui_font_fa13.cyclo ./Core/Src/eez/ui_font_fa13.d ./Core/Src/eez/ui_font_fa13.o ./Core/Src/eez/ui_font_fa13.su ./Core/Src/eez/ui_font_fa14.cyclo ./Core/Src/eez/ui_font_fa14.d ./Core/Src/eez/ui_font_fa14.o ./Core/Src/eez/ui_font_fa14.su ./Core/Src/eez/ui_font_fa16.cyclo ./Core/Src/eez/ui_font_fa16.d ./Core/Src/eez/ui_font_fa16.o ./Core/Src/eez/ui_font_fa16.su ./Core/Src/eez/ui_font_fa18.cyclo ./Core/Src/eez/ui_font_fa18.d ./Core/Src/eez/ui_font_fa18.o ./Core/Src/eez/ui_font_fa18.su ./Core/Src/eez/ui_font_fa20.cyclo ./Core/Src/eez/ui_font_fa20.d ./Core/Src/eez/ui_font_fa20.o ./Core/Src/eez/ui_font_fa20.su ./Core/Src/eez/ui_font_fa24.cyclo ./Core/Src/eez/ui_font_fa24.d ./Core/Src/eez/ui_font_fa24.o ./Core/Src/eez/ui_font_fa24.su ./Core/Src/eez/ui_font_rl104.cyclo ./Core/Src/eez/ui_font_rl104.d ./Core/Src/eez/ui_font_rl104.o ./Core/Src/eez/ui_font_rl104.su ./Core/Src/eez/ui_font_rl26.cyclo ./Core/Src/eez/ui_font_rl26.d ./Core/Src/eez/ui_font_rl26.o ./Core/Src/eez/ui_font_rl26.su ./Core/Src/eez/ui_font_rm10.cyclo ./Core/Src/eez/ui_font_rm10.d ./Core/Src/eez/ui_font_rm10.o ./Core/Src/eez/ui_font_rm10.su ./Core/Src/eez/ui_font_rm11.cyclo ./Core/Src/eez/ui_font_rm11.d ./Core/Src/eez/ui_font_rm11.o ./Core/Src/eez/ui_font_rm11.su ./Core/Src/eez/ui_font_rm13.cyclo ./Core/Src/eez/ui_font_rm13.d ./Core/Src/eez/ui_font_rm13.o ./Core/Src/eez/ui_font_rm13.su ./Core/Src/eez/ui_font_rm14.cyclo ./Core/Src/eez/ui_font_rm14.d ./Core/Src/eez/ui_font_rm14.o ./Core/Src/eez/ui_font_rm14.su ./Core/Src/eez/ui_font_rm15.cyclo ./Core/Src/eez/ui_font_rm15.d ./Core/Src/eez/ui_font_rm15.o ./Core/Src/eez/ui_font_rm15.su ./Core/Src/eez/ui_font_rm26.cyclo ./Core/Src/eez/ui_font_rm26.d ./Core/Src/eez/ui_font_rm26.o ./Core/Src/eez/ui_font_rm26.su ./Core/Src/eez/ui_font_rm9.cyclo ./Core/Src/eez/ui_font_rm9.d ./Core/Src/eez/ui_font_rm9.o ./Core/Src/eez/ui_font_rm9.su ./Core/Src/eez/ui_font_rr11.cyclo ./Core/Src/eez/ui_font_rr11.d ./Core/Src/eez/ui_font_rr11.o ./Core/Src/eez/ui_font_rr11.su ./Core/Src/eez/ui_font_rr13.cyclo ./Core/Src/eez/ui_font_rr13.d ./Core/Src/eez/ui_font_rr13.o ./Core/Src/eez/ui_font_rr13.su ./Core/Src/eez/ui_font_rr14.cyclo ./Core/Src/eez/ui_font_rr14.d ./Core/Src/eez/ui_font_rr14.o ./Core/Src/eez/ui_font_rr14.su ./Core/Src/eez/ui_font_rr15.cyclo ./Core/Src/eez/ui_font_rr15.d ./Core/Src/eez/ui_font_rr15.o ./Core/Src/eez/ui_font_rr15.su ./Core/Src/eez/ui_font_rr20.cyclo ./Core/Src/eez/ui_font_rr20.d ./Core/Src/eez/ui_font_rr20.o ./Core/Src/eez/ui_font_rr20.su ./Core/Src/eez/ui_font_rr22.cyclo ./Core/Src/eez/ui_font_rr22.d ./Core/Src/eez/ui_font_rr22.o ./Core/Src/eez/ui_font_rr22.su ./Core/Src/eez/ui_image_tc_fireside.cyclo ./Core/Src/eez/ui_image_tc_fireside.d ./Core/Src/eez/ui_image_tc_fireside.o ./Core/Src/eez/ui_image_tc_fireside.su

.PHONY: clean-Core-2f-Src-2f-eez


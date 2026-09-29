import glob
for f in glob.glob('STM32F405RGT6-display-rfid-barcode/Core/Src/eez/*.h'):
    with open(f, 'r') as file:
        content = file.read()
    content = content.replace('<lvgl/lvgl.h>', '"lvgl.h"')
    with open(f, 'w') as file:
        file.write(content)

# EC FW

## Setup
```bash
.venv\Scripts\activate.bat

set ZES_ENABLE_SYSMAN=1
set ZEPHYR_SDK_INSTALL_DIR=C:\Users\Andy\Desktop\app_mplab\zephyr-sdk-0.17.0

set MEC172X_SPI_GEN=C:\Users\Andy\Desktop\I3C\CPGZephyrDocs\MEC172x\SPI_image_gen\mec172x_spi_gen_win.exe
set PATH=%PATH%;C:\Program Files (x86)\DediProg\SF Programmer
set PATH=%PATH%;C:\msys64\ucrt64\bin

zephyr_3.2.0\zephyr\zephyr-env.cmd
```

## Build
```bash
west build -b mec1723_n1x develope\app\ec_fw

# west build -p -b mec1723_n1x develope\app\ec_fw -DBOARD_ROOT="C:\Users\Andy\Desktop\app_mplab\develope\app\ec_fw"
```

## Flash
```bash
DpCmd.exe -d
DpCmd.exe -u build\zephyr\spi_image.bin
```

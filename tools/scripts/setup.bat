
call .venv\Scripts\activate.bat

set ZES_ENABLE_SYSMAN=1
set ZEPHYR_TOOLCHAIN_VARIANT=zephyr
set ZEPHYR_SDK_INSTALL_DIR=C:\Users\Andy\Desktop\app_mplab\zephyr-sdk-0.17.0
set WORKSPACE=%cd%

set MEC172X_SPI_GEN=%WORKSPACE%\tools\CPGZephyrDocs\MEC172x\SPI_image_gen\mec172x_spi_gen_win.exe
set EC_IMG_GEN=%WORKSPACE%\tools\spi_image_trim_out\generating_binaries.py
@REM set PATH=%PATH%;C:\Program Files (x86)\DediProg\SF Programmer
@REM set PATH=%PATH%;C:\msys64\ucrt64\bin

zephyr_3.7.0\zephyr\zephyr-env.cmd
@REM zephyr_3.2.0\zephyr\zephyr-env.cmd


echo ================ Zephyr ENV =======================
echo ZES_ENABLE_SYSMAN=%ZES_ENABLE_SYSMAN%
echo ZEPHYR_TOOLCHAIN_VARIANT=%ZEPHYR_TOOLCHAIN_VARIANT%
echo ZEPHYR_SDK_INSTALL_DIR=%ZEPHYR_SDK_INSTALL_DIR%
echo ================= MCHP ENV ========================
echo MEC172X_SPI_GEN=%MEC172X_SPI_GEN%
echo EC_IMG_GEN=%EC_IMG_GEN%

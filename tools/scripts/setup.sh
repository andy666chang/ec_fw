#!/bin/bash

# --- 1. 啟用 Python 虛擬環境 ---
# 注意：這假定您已經像之前的腳本那樣創建了 .venv 資料夾。
source .venv/bin/activate

# --- 2. 設定 Zephyr 相關環境變數 ---

export ZES_ENABLE_SYSMAN=1
export ZEPHYR_TOOLCHAIN_VARIANT=zephyr

# --- 3. 設定工作區路徑與工具路徑 ---

# 在 macOS/Linux 中，使用 $(pwd) 或 `pwd` 獲取當前工作目錄
WORKSPACE=$(pwd)
export WORKSPACE

# 使用正斜線 / 作為路徑分隔符
export ZEPHYR_SDK_INSTALL_DIR="$WORKSPACE/zephyr-sdk-0.17.0"
export MEC172X_SPI_GEN="$WORKSPACE/tools/CPGZephyrDocs/MEC172x/SPI_image_gen/mec172x_spi_gen_lin_x86_64"
export EC_IMG_GEN="$WORKSPACE/tools/spi_image_trim_out/generating_binaries.py"

# 給予編譯器權限 ＆ 清除cmake cache
# xattr -r -d com.apple.quarantine "$ZEPHYR_SDK_INSTALL_DIR"
# rm -rf ~/Library/Caches/zephyr 
# ref: https://community.circuitdojo.com/d/16-the-toolchain-is-unable-to-build-a-dummy-c-file-mac-os

# --- 4. 修改 PATH 變數 (註解部分) ---
# 在 Windows 中使用 %PATH%;C:\... 來追加路徑。
# 在 macOS/Linux 中使用 $PATH:... 來追加路徑。
# export PATH="$PATH:/C/Program Files (x86)/DediProg/SF Programmer"
# export PATH="$PATH:/C/msys64/ucrt64/bin"


# --- 5. 執行 Zephyr 環境設定腳本 ---
# Zephyr 預設提供了 bash 版本的環境設定檔，我們需要 source 它來載入 SDK 路徑等。
# 請根據您的實際 Zephyr 版本修改以下路徑，並確保該檔案存在。
source zephyr_3.7.0/zephyr_kernel/zephyr-env.sh


# --- 6. 顯示當前環境變數以供驗證 ---

echo "================ Zephyr ENV ======================="
echo "ZES_ENABLE_SYSMAN=$ZES_ENABLE_SYSMAN"
echo "ZEPHYR_TOOLCHAIN_VARIANT=$ZEPHYR_TOOLCHAIN_VARIANT"
echo "ZEPHYR_SDK_INSTALL_DIR=$ZEPHYR_SDK_INSTALL_DIR"
echo "================= MCHP ENV ========================"
echo "MEC172X_SPI_GEN=$MEC172X_SPI_GEN"
echo "EC_IMG_GEN=$EC_IMG_GEN"


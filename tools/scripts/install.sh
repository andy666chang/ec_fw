#!/bin/bash

# 建立 Python 虛擬環境
python3 -m venv .venv

# 啟動虛擬環境 (使用 source 命令)
# 在 Windows 的 .bat 中使用 call .venv\Scripts\activate.bat
source .venv/bin/activate

# 安裝 west 工具
pip3 install west

# 進入 tools 目錄，使用 -l 參數初始化 west，然後返回上一層
cd tools
west init -l
cd ..

# 更新 west 模組和專案
west update

# 匯出 zephyr 環境變數
west zephyr-export

# 安裝 Zephyr 核心的 Python 需求套件
pip install -r zephyr_3.7.0/zephyr_kernel/scripts/requirements.txt

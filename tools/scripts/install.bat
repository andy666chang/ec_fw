
python -m venv .venv

call .venv\Scripts\activate.bat

pip install west

west init zephyr_3.7.0 --mr v3.7.0
cd zephyr_3.7.0
west update

west zephyr-export

pip install -r zephyr\scripts\requirements.txt

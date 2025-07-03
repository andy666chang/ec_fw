
python -m venv .venv

call .venv\Scripts\activate.bat

pip install west

@REM west init zephyr_3.7.0 --mr v3.7.0
@REM cd zephyr_3.7.0

cd tools
west init -l
cd ..
west update

west zephyr-export

pip install -r zephyr\scripts\requirements.txt

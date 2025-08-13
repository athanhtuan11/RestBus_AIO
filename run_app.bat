@echo off
cd /d %~dp0
call conda activate restbusenv
python main.py
pause

@echo off
cd /d E:\STM32project\rm_2027\mas_embedded_threadx-f103
cmake --build build\f103_c8\Debug --config Debug -j 4 2>&1
echo EXITCODE:%ERRORLEVEL%

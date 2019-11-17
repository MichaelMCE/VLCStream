@echo off

rem call gccpath


call make WIN64=1 -j10 vlcstream.exe

call copy vlcstream.exe "C:\Program Files (x86)\vlc-3.0.0\vlcstream.exe" /y


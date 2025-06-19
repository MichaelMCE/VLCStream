@echo off

rem call gccpath

call make xcpy SDK=0
call make -j12 SDK=3222 APP_TYPE=windows vlcstream.exe


rem call copy vlcstream.exe "C:\Program Files (x86)\vlc-2.2.0\vlcstream.exe" /y


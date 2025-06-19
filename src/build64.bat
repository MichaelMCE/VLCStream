@echo off

rem call gccpath

call make xcpy SDK=0
call make -j12 WIN64=1 SDK=6419 APP_TYPE=windows vlcstream.exe

rem call copy vlcstream.exe "C:\Program Files\vlcStream64\_vlcstream.exe" /y


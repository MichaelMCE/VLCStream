@echo off

rem call gccpath


call make xcpy
call make WIN64=1 -j12 vlcstream.exe

call copy vlcstream.exe "C:\Program Files\vlcStream64\_vlcstream.exe" /y


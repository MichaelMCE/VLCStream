@echo off

rem call gccpath

call make xcpy SDK=0
call make WIN64=1 SDK=6419 -j12 vlcstream.exe

rem call copy vlcstream.exe "C:\Program Files\vlcStream64\_vlcstream.exe" /y


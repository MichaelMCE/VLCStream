@echo off

rem call gccpath

call make xcpy SDK=0
call make -j12 SDK=3222 vlcstream.exe

del _vlcstream.exe
rename vlcstream.exe _vlcstream.exe

call copy _vlcstream.exe "C:\Program Files (x86)\vlc-2.2.0\_vlcstream.exe" /y


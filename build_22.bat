@echo off

rem call gccpath

del _vlcstream.exe
call make -j10 vlcstream.exe
rename vlcstream.exe _vlcstream.exe
call copy _vlcstream.exe "C:\Program Files (x86)\vlc-2.2.0\_vlcstream.exe" /y



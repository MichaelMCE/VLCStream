@echo off

rem call gccpath

del _vlcstream.exe
call make xcpy
call make -j10 vlcstream.exe
rename vlcstream.exe _vlcstream.exe

rem call copy _vlcstream.exe "C:\Program Files (x86)\VLC\_vlcstream.exe" /y
rem call copy _vlcstream.exe "C:\Program Files (x86)\vlcStream32\_vlcstream.exe" /y
call copy _vlcstream.exe "C:\Program Files (x86)\vlc-2.2.0\_vlcstream.exe" /y



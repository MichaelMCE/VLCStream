@echo off

rem call gccpath

del _vlcstream.exe
call make xcpy
call make -j10 vlcstream.exe
rename vlcstream.exe _vlcstream.exe

call copy _vlcstream.exe "C:\Program Files (x86)\VLC\_vlcstream.exe" /y
call copy _vlcstream.exe "M:\RamDiskTemp\vlc\_vlcstream.exe" /y



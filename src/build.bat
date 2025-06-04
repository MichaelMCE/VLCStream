@echo off

rem call gccpath

call make xcpy
call make -j12 vlcstream.exe

rem call copy vlcstream.exe "C:\Program Files (x86)\VLC\vlcstream.exe" /y
rem call copy vlcstream.exe "M:\RamDiskTemp\vlc\vlcstream.exe" /y



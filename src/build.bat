@echo off

rem call gccpath

call make xcpy
call make -j10 vlcstream.exe

call copy vlcstream.exe "C:\Program Files (x86)\VLC\vlcstream.exe" /y
call copy vlcstream.exe "M:\RamDiskTemp\vlc\vlcstream.exe" /y



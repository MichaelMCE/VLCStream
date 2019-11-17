@echo off

k:
cd K:\code\mylcd\examples\VLC

call gccpath

del _vlcstream.exe
del vlcstream.exe
del res.o

call make -j10 vlcstream.exe
rename vlcstream.exe _vlcstream.exe
call copy _vlcstream.exe "C:\Program Files (x86)\VLC\_vlcstream.exe" /y




rem pause
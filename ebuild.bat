@echo off

k:
cd K:\code\mylcd\examples\VLC

call gccpath

del res.o
del vlcstream.exe
del _vlcstream.exe

call make -j10 vlcstream.exe
call copy vlcstream.exe "C:\Program Files (x86)\VLC\vlcstream.exe" /y


rem pause
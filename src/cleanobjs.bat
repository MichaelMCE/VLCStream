@echo off

del /S *.o
del /S *.bak

del *.s
del *.def
rem del *.a
rem del mylcd.dll
rem del libmylcd.a
del libmylcd.a
del *.bmp
del *.tga
del *.pgm
del *.png
del *.raw
del *.exe	

cd cc
call clean.bat
cd ..

rem cd opengl
rem call clean.bat
rem cd ..

cd tetris
call clean.bat
cd ..

cd antplus
call clean.bat
cd..

cd mxml
call clean.bat
cd..







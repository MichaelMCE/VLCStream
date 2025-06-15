@echo off

rem call gccpath

call make SDK=0 xcpy
call make SDK=0 cleanextra
call make SDK=0 WIN64=1 extra


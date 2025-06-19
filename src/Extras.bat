@echo off

rem call gccpath

call make SDK=0 xcpy
call make SDK=0 cleanextra
call make SDK=0 WIN64=1 APP_TYPE=windows extra
call make SDK=0 cleanextra

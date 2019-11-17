rem @echo off

echo compiling....

rem call gccpath.bat
@gcc -shared -O2 -m32 -D_WIN32_WINNT=0x0601 hook.c -o hook.dll -static-libgcc  -static-libstdc++ -DBUILDING_DLL=1 --pipe -Wl,--out-implib,libhook.a


strip hook.dll



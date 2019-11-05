@echo off

xcopy "M:\RamDiskTemp\vlc\vsskin\config.cfg" "C:\Program Files (x86)\VLC\vsskin\" /y /d
xcopy "M:\RamDiskTemp\vlc\vlcsplaylist.m3u8" "C:\Program Files (x86)\VLC\" /y /d

@echo Copying VLC to Ramdisk ..
xcopy "C:\Program Files (x86)\VLC" "M:\RamDiskTemp\vlc" /y /d /I /S

@echo .    

"C:\Program Files (x86)\VLC\vlcsCtrl.exe" flush

rem pause




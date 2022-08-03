@echo off
pushd %~dp0\..\
call Vendor\premake5.exe --file=MainMake.lua vs2022
popd
pause
@echo off
set prg=lcryp
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\msbuild.exe" build_msvc\%prg%.sln -property:Configuration=Release -maxCpuCount -verbosity:minimal
rd /s /q LcRyp Core
MD LcRyp Core
MD LcRyp Core\daemon
copy "build_msvc\x64\Release\%prg%-qt.exe" "LcRyp Core\%prg%-qt.exe"
copy "build_msvc\x64\Release\%prg%-cli.exe" "LcRyp Core\daemon\%prg%-cli.exe"
copy "build_msvc\x64\Release\%prg%-tx.exe" "LcRyp Core\daemon\%prg%-tx.exe"
copy "build_msvc\x64\Release\%prg%d.exe" "LcRyp Core\daemon\%prg%d.exe"
pause
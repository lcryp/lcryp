@echo off
set prg=lcryp
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\msbuild.exe" build_msvc\%prg%.sln -property:Configuration=Release -maxCpuCount -verbosity:minimal
rd /s /q LcRyp-Wallet
MD LcRyp-Wallet
MD LcRyp-Wallet\daemon
copy "build_msvc\x64\Release\%prg%-qt.exe" "LcRyp-wallet\%prg%-qt.exe"
copy "build_msvc\x64\Release\%prg%-cli.exe" "LcRyp-wallet\daemon\%prg%-cli.exe"
copy "build_msvc\x64\Release\%prg%-tx.exe" "LcRyp-wallet\daemon\%prg%-tx.exe"
copy "build_msvc\x64\Release\%prg%d.exe" "LcRyp-wallet\daemon\%prg%d.exe"
pause
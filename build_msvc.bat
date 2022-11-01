@echo off
set prg=lcryp
"F:\ProgramingFiles\Microsoft Visual Studio\2022\MSBuild\Current\Bin\msbuild.exe" build_msvc\%prg%.sln -property:Configuration=Release -maxCpuCount -verbosity:minimal
rd /s /q LcRyp-wallet
MD LcRyp-wallet
MD LcRyp-wallet\daemon
MD LcRyp-wallet\other
copy "build_msvc\x64\Release\%prg%-qt.exe" "LcRyp-wallet\%prg%-qt.exe"
copy "build_msvc\x64\Release\%prg%-cli.exe" "LcRyp-wallet\daemon\%prg%-cli.exe"
copy "build_msvc\x64\Release\%prg%-tx.exe" "LcRyp-wallet\daemon\%prg%-tx.exe"
copy "build_msvc\x64\Release\%prg%d.exe" "LcRyp-wallet\daemon\%prg%d.exe"
copy "build_msvc\x64\Release\generate_genesis.exe" "LcRyp-wallet\other\generate_genesis.exe"
rem rd /s /q build_msvc\x64
@echo ok>build_final
pause
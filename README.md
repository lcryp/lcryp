LCRYP CORE INTEGRATION/STAGING TREE
===================================

[Official site](https://www.lcryp.com) (the site is still under development)

What is LcRyp Core?
-------------------

LcRyp Core connects to the LcRyp peer-to-peer network to download and fully
validate blocks and transactions. It also includes a wallet and graphical user
interface, which can be optionally built. And the possibility of mining on 
your personal computer.

License
-------

LcRyp Core is released under the terms of the MIT license. See Copyright (c) 2022 LcRyp for more
information or see https://lcryp.com/lcryp-core-license.html

Development Process
-------------------

The `master` branch is regularly built and tested, but it is not guaranteed to be
completely stable. [Tags](https://github.com/lcryp/lcryp/tags) are created
regularly from release branches to indicate new official, stable release versions of LcRyp Core.

WINDOWS BUILD NOTES
====================

Below are some notes on how to build LcRyp Core for Windows.

The options known to work for building LcRyp Core on Windows are:

On Windows, using [Microsoft Visual Studio](https://www.visualstudio.com)

Building LcRyp Core with Visual Studio
--------------------------------------

### Introduction

Visual Studio 2022 is minimum required to build LcRyp Core.

Solution and project files to Visual Studio can be found in the `build_msvc` directory.

To build LcRyp Core from the command-line, it is sufficient to only install the Visual Studio Build Tools component.

** Catalogs by default (the explanation will be in the text below) **

* Visual Studio 2022 `C:\Programing Files\Microsoft Visual Studio\2022`
* Source codes LcRyp `C:\LcRyp\lctyp-master`
* Vcpkg ready assembly `C:\vcpkg`
* QT source codes `C:\qt-everywhere-src-5.15.6`
* QT ready assembly `C:\qt-static-5.15.6`
* Python `C:\Python38\`

** Environment Variables **

* `QTBASEDIR` `C:\qt-static-5.15.6\`
* `PYTHONUTF8` `1`

###  Installation Python

** [Install](https://www.python.org) Python [3.8]. Python Installation folder `C:\Python38\` **

### Installation Vcpkg

To build [dependencies] (except for [Qt](#qt)), the default approach is to use the [vcpkg](https://docs.microsoft.com/en-us/cpp/vcpkg) package manager from Microsoft:

** 1. [Install](https://www.vcpkg.io/en/getting-started.html) vcpkg. **
Create a bat file `C:\vcpkg\bootstrap-vcpkg.bat`

``` 
@echo off
%comspec% /k "C:\Programing Files\Microsoft Visual Studio\2022\VC\Auxiliary\Build\vcvars64.bat"
.\vcpkg.exe update
.\vcpkg.exe integrate install
.\vcpkg.exe install --clean-after-build --triplet x64-windows-static --x-manifest-root "C:\LcRyp\lctyp-master\build_msvc" 
pause
```

** 2. By default, vcpkg makes both `release` and `debug` builds for each package.**
To save build time and disk space, one could skip `debug` builds (example uses PowerShell, let's set the environment variable): 
```powershell
Add-Content -Path "vcpkg\triplets\x64-windows-static.cmake" -Value "set(VCPKG_BUILD_TYPE release)"
```

### Installation Qt

To build LcRyp Core with the GUI, a static build of Qt is required.

** 1. Download a single ZIP archive of Qt source code ** 
from https://download.qt.io/official_releases/qt/ 
(e.g., [`qt-everywhere-opensource-src-5.15.6.zip`](https://download.qt.io/official_releases/qt/5.15/5.15.6/single/qt-everywhere-opensource-src-5.15.6.zip)), 
(this is the maximum supported version, higher version does not work.) and expand it into a dedicated folder. 
The following instructions assume that this folder is `C:\qt-everywhere-src-5.15.6`.

** 2. Open "x64 Native Tools Command Prompt for VS 2022", and input the following commands or create a bat file: ** 
Create a bat file `C:\qt-everywhere-src-5.15.6\install-qt-everywhere-src-5.15.6.bat`

```
@echo off
%comspec% /k "F:\ProgramingFiles\Microsoft Visual Studio\2022\VC\Auxiliary\Build\vcvars64.bat"
cd C:\qt-everywhere-src-5.15.6
mkdir build
cd C:\qt-everywhere-src-5.15.6\build\
..\configure -release -silent -opensource -confirm-license -opengl desktop -no-shared -static -static-runtime -mp -qt-zlib -qt-pcre -qt-libpng -no-libjpeg -nomake examples -nomake tests -nomake tools -no-dbus -no-libudev -no-icu -no-gtk -no-opengles3 -no-angle -no-sql-sqlite -no-sql-odbc -no-sqlite -no-libudev -no-vulkan -skip qt3d -skip qtactiveqt -skip qtandroidextras -skip qtcanvas3d -skip qtcharts -skip qtconnectivity -skip qtdatavis3d -skip qtdeclarative -skip qtdoc -skip qtgamepad -skip qtgraphicaleffects -skip qtimageformats -skip qtlocation -skip qtmacextras -skip qtmultimedia -skip qtnetworkauth -skip qtpurchasing -skip qtquickcontrols -skip qtquickcontrols2 -skip qtscript -skip qtscxml -skip qtsensors -skip qtserialbus -skip qtserialport -skip qtspeech -skip qtvirtualkeyboard -skip qtwayland -skip qtwebchannel -skip qtwebengine -skip qtwebsockets -skip qtwebview -skip qtx11extras -skip qtxmlpatterns -no-openssl -no-feature-sql -no-feature-sqlmodel -prefix "C:\qt-static-5.15.6"
nmake
nmake install
pause
```

** 3. Create environment variables ** 
`QTBASEDIR` `C:\qt-static-5.15.6\` 

*** To build LcRyp Core without Qt, unload or disable the `lcryp-qt`, `liblcryp_qt` and `test_lcryp-qt` projects.***

### Building

**1. To generate the project files `*.vcxproj`**
For the Visual Studio 2022 in accordance with the configurations, it is necessary to run the python script toolchain from Makefile:

```cmd
python build_msvc\msvc-autogen.py
```

** 2. An optional step is to adjust the settings** 
In the `build_msvc` directory and the `common.init.vcxproj` file. 
This project file contains settings that are common to all projects such as the runtime library version and target Windows SDK version. 
The Qt directories can also be set. To specify a non-default path to a static Qt package directory, use the `QTBASEDIR` environment variable.

** 3. To build from the command-line with the Visual Studio toolchain use:**
Create a bat file `C:\LcRyp\lctyp-master\build_msvc.bat`

```
@echo off
"C:\Programing Files\Microsoft Visual Studio\2022\MSBuild\Current\Bin\msbuild.exe" build_msvc\lcryp.sln -property:Configuration=Release -maxCpuCount -verbosity:minimal
pause
```

*** Alternatively, open the `build_msvc/lcryp.sln` file in Visual Studio. ***

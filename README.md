LCRYP CORE INTEGRATION/STAGING TREE
===================================

[Official site](https://www.lcryp.com) (the site is still under development)

* [What is LcRyp Core?](#what-is-lcryp-core)
* [License](#license)
* [Development Process](#development-process)

What is LcRyp Core?
-------------------

LcRyp Core connects to the LcRyp peer-to-peer network to download and fully
validate blocks and transactions. It also includes a wallet and graphical user
interface, which can be optionally built. And the possibility of mining on 
your personal computer.

License
-------

LcRyp Core is released under the terms of the MIT license. For more information or see [Copyright (c) 2022 LcRyp](https://lcryp.com/lcryp-core-license.html)

Development Process
-------------------

The `master` branch is regularly built and tested, but it is not guaranteed to be
completely stable. [Tags](https://github.com/lcryp/lcryp/tags) are created
regularly from release branches to indicate new official, stable release versions of LcRyp Core.

WINDOWS BUILD NOTES
====================

Below are some notes on how to build LcRyp Core for Windows x64.

The options known to work for building LcRyp Core on Windows are:

On Windows x64, using [Microsoft Visual Studio 2022](https://www.visualstudio.com)

Building LcRyp Core with Visual Studio
--------------------------------------

* [Introduction](#introduction)
* [Download the Microsoft Visual Studio 2022](#download-the-microsoft-visual-studio-2022)
* [Download the project LcRyp source codes](#download-the-project-lcryp-source-codes)
* [Installation Python](#installation-python)
* [Installation Vcpkg](#installation-vcpkg)
* [Installation Qt](#installation-qt)
* [Building](#building)

### Introduction

Visual Studio 2022 is minimum required to build LcRyp Core.

Solution and project files to Visual Studio can be found in the `build_msvc` directory.

To build LcRyp Core from the command-line, it is sufficient to only install the Visual Studio Build Tools component.

**Catalogs by default (the explanation will be in the text below)**

* Visual Studio 2022 `С:\Program Files\Microsoft Visual Studio\2022\`
* Source codes LcRyp `C:\LcRyp\lcryp-master\`
* Vcpkg ready assembly `C:\vcpkg\`
* QT source codes `C:\qt-everywhere-src-5.15.6\`
* QT ready assembly `C:\qt-static-5.15.6\`
* Python `C:\Python38\`

**Environment Variables**

* `QTBASEDIR` `C:\qt-static-5.15.6\`
* `PYTHONUTF8` `1`
* `Path` add to existing line `C:\Python38\Scripts\;C:\Python38\;` If it is not automatically added during installation, install it yourself. Just check.

### Download the Microsoft Visual Studio 2022

* Download [Microsoft Visual Studio 2022](https://visualstudio.microsoft.com/thank-you-downloading-visual-studio/?sku=Community&channel=Release&version=VS2022&source=VSLandingPage&cid=2030&passive=false)
* Configure component installation: `Classic and mobile apps` -> `C++ desktop development`
* Use a folder `С:\Program Files\Microsoft Visual Studio\2022\`

### Download the project LcRyp source codes

* Download source codes [lcryp-master](https://github.com/lcryp/LcRyp/archive/refs/heads/master.zip)
* Use a folder `C:\LcRyp\lcryp-master\`

###  Installation Python

For the correct assembly of projects, it is necessary to be able to execute python scripts.
* Get started with [Python](https://www.python.org)
* Python 3.8 [Install](https://www.python.org/ftp/python/3.8.0/python-3.8.0-amd64.exe) 
* Python installation folder `C:\Python38\`
* Create environment variables `PYTHONUTF8` `1` and `Path` add to existing line `C:\Python38\Scripts\;C:\Python38\;`

```cmd
setx PYTHONUTF8 1 /M 
setx Path "%Path%;C:\Python38\Scripts\;C:\Python38\" /M 
```

### Installation Vcpkg

To build [dependencies] (except for [Qt](#installation-qt)), the default approach is to use Vcpkg:

**1. Download a single ZIP archive of vcpkg source code** 
* About [vcpkg](https://docs.microsoft.com/en-us/cpp/vcpkg) package manager from Microsoft
* Get started with [vcpkg](https://www.vcpkg.io/en/getting-started.html)
* From official [source code](https://github.com/microsoft/vcpkg)
* Download the archive [`vcpkg-master.zip`](https://github.com/microsoft/vcpkg/archive/refs/heads/master.zip)
* And expand it into a dedicated folder `C:\vcpkg`

**2. Create a bat file `C:\vcpkg\install-vcpkg.bat` and execute it**

```cmd
@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass "& {& '%~dp0scripts\bootstrap.ps1' %*}"
%comspec% /k "С:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
.\vcpkg.exe update
.\vcpkg.exe integrate install
.\vcpkg.exe install --clean-after-build --triplet x64-windows-static --x-manifest-root "C:\LcRyp\lctyp-master\build_msvc" 
pause
```

**3. By default, vcpkg makes both `release` and `debug` builds for each package.**
To save build time and disk space, one could skip `debug` builds (example uses PowerShell, let's set the environment variable): 
```powershell
Add-Content -Path "C:\vcpkg\triplets\x64-windows-static.cmake" -Value "set(VCPKG_BUILD_TYPE release)"
```

### Installation Qt

To build LcRyp Core with the GUI, a static build of Qt is required.

**1. Download a single ZIP archive of Qt source code** 
* From official [releases](https://download.qt.io/official_releases/qt/)
* Qt 5.15.6 [`qt-everywhere-opensource-src-5.15.6.zip`](https://download.qt.io/official_releases/qt/5.15/5.15.6/single/qt-everywhere-opensource-src-5.15.6.zip)
***Qt 5.15.6 this is the maximum supported version, higher version does not work.*** 
* And expand it into a dedicated folder is `C:\qt-everywhere-src-5.15.6`

**2. Open "x64 Native Tools Command Prompt for VS 2022", and input the following commands or create a bat file:** 
Create a bat file `C:\qt-everywhere-src-5.15.6\install-qt.bat` and execute it.

```cmd
@echo off
%comspec% /k "С:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
pause
```

After running this file, a console window will open in which you need to enter the following 5 lines in sequence:
```mkdir C:\qt-everywhere-src-5.15.6\build\```
```cd C:\qt-everywhere-src-5.15.6\build\```
```..\configure -release -silent -opensource -confirm-license -opengl desktop -no-shared -static -static-runtime -mp -qt-zlib -qt-pcre -qt-libpng -no-libjpeg -nomake examples -nomake tests -nomake tools -no-dbus -no-libudev -no-icu -no-gtk -no-opengles3 -no-angle -no-sql-sqlite -no-sql-odbc -no-sqlite -no-libudev -no-vulkan -skip qt3d -skip qtactiveqt -skip qtandroidextras -skip qtcanvas3d -skip qtcharts -skip qtconnectivity -skip qtdatavis3d -skip qtdeclarative -skip qtdoc -skip qtgamepad -skip qtgraphicaleffects -skip qtimageformats -skip qtlocation -skip qtmacextras -skip qtmultimedia -skip qtnetworkauth -skip qtpurchasing -skip qtquickcontrols -skip qtquickcontrols2 -skip qtscript -skip qtscxml -skip qtsensors -skip qtserialbus -skip qtserialport -skip qtspeech -skip qtvirtualkeyboard -skip qtwayland -skip qtwebchannel -skip qtwebengine -skip qtwebsockets -skip qtwebview -skip qtx11extras -skip qtxmlpatterns -no-openssl -no-feature-sql -no-feature-sqlmodel -prefix "C:\qt-static-5.15.6"```
```nmake```
```nmake install```

**3. Create environment variables** 
`QTBASEDIR` `C:\qt-static-5.15.6\` 

```cmd
setx QTBASEDIR "C:\qt-static-5.15.6\"  /M 
```

***To build LcRyp Core without Qt, unload or disable the `lcryp-qt` and `liblcryp_qt` projects.***

### Building

**1. To generate the project files `*.vcxproj`**
For the Visual Studio 2022 in accordance with the configurations, it is necessary to run the python script toolchain from Makefile. To do this, enter the following in the command line:

```cmd
"C:\Python38\python.exe" "C:\LcRyp\lctyp-master\build_msvc\msvc-autogen.py"
```

**2. An optional step is to adjust the settings** 
In the `build_msvc` directory and the `common.init.vcxproj` file. 
This project file contains settings that are common to all projects such as the runtime library version and target Windows SDK version. 
The Qt directories can also be set. To specify a non-default path to a static Qt package directory, use the `QTBASEDIR` environment variable.

**3. To build from the command-line with the Visual Studio toolchain use:**
Create a bat file `C:\LcRyp\lctyp-master\build_msvc.bat`

```cmd
@echo off
"C:\Programing Files\Microsoft Visual Studio\2022\MSBuild\Current\Bin\msbuild.exe" build_msvc\lcryp.sln -property:Configuration=Release -maxCpuCount -verbosity:minimal
pause
```

Alternatively, open the `C:\LcRyp\lctyp-master\build_msvc\build_msvc\lcryp.sln` file in Visual Studio 2022.

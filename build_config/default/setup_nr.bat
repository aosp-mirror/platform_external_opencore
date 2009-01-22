@echo OFF
IF (%1)==() echo.Usage !!!ERROR!!! %0 {Base Directory} Use capital letter for drive letter.

rem *** Setup environment for necessary for the makefiles, etc. ***

rem *** All build output goes under BUILD_ROOT including object files, dependency files, libraries, installed headers, etc ***
set BUILD_ROOT=%PROJECT_DIR%\build
echo Set BUILD_ROOT to %BUILD_ROOT%

rem *** BASE_DIR is the base directory mainly to specify MK. ***
rem *** Helps us to set path for some of the extern_tools required in the build. ***
set BASE_DIR=%1
echo Set BASE_DIR to %BASE_DIR%

rem *** SRC_ROOT is the root of all the sources ***
set SRC_ROOT=%BASE_DIR%
echo Set SRC_ROOT to %SRC_ROOT%

rem *** MK is where all the makefile templates are picked from. ***
set MK=%BASE_DIR%\tools_v2\build\make_nr\tools_v2\build\make
echo Set MK to %MK%

rem *** Append win32 make.exe path in PATH ***
set PATH=%BASE_DIR%\extern_tools_v2\bin\win32;C:\Program Files\Microsoft Visual Studio\Common\MSDev98\Bin;C:\Program Files\Microsoft Visual Studio\VC98\Bin;C:\Program Files\Microsoft Visual Studio 8\Common7\IDE;c:\Program Files\Microsoft Visual Studio 8\Common7\Tools;%PATH%

rem *** TOOLSET is used to determine which tools to use ***
set TOOLSET=vc6
echo Set TOOLSET to %TOOLSET%

rem *** PLATFORM_EXTRAS is used to include platform specific templates ***
set PLATFORM_EXTRAS=%MK%\projfile.mk
echo Set PLATFORM_EXTRAS to %PLATFORM_EXTRAS%


echo ******************************************
echo Environment is ready if no errors reported
echo ******************************************


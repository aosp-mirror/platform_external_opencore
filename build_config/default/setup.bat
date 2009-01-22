@echo OFF
IF (%1)==() echo.Usage ERROR! %0 VOB base directory "use double slashes" eg: C:\\static_view_dir [Static] D:\ [Dynamic]

rem *** Setup environment for necessary for the makefiles, etc. ***

rem *** Set this flag for integarting CML2 config with the builds ***
set USE_CML2_CONFIG=1

rem *** Set the VOB_BASE_DIR ***
set VOB_BASE_DIR=%1
echo Set VOB_BASE_DIR to %VOB_BASE_DIR% ...

rem *** Set the PV_TOP ***
set PV_TOP=%VOB_BASE_DIR%\\oscl
echo Set PV_TOP to %PV_TOP% ...

rem *** Set the PROJECT ***
set PROJECT=%PV_TOP%
echo Set PROJECT to %PROJECT% ...

rem *** Set MK to path keeping the makefiles templates ***
set MK=%VOB_BASE_DIR%\tools_v2\build\make
echo Set MK to %MK% ...

rem *** Append win32 make.exe path in PATH ***

rem *** Set Defaults Path for Perl bin directory ***
set PERL_HOME_PATH=C:\Perl\bin
set PATH=%VOB_BASE_DIR%\extern_tools_v2\bin\win32;C:\Program Files\Microsoft Visual Studio\Common\MSDev98\Bin;C:\Program Files\Microsoft Visual Studio\VC98\Bin;C:\Program Files\Microsoft Visual Studio 8\Common7\IDE;c:\Program Files\Microsoft Visual Studio 8\Common7\Tools;c:\windows;c:\windows\system32;C:\Windows\System32\Wbem;C:\Program Files\Rational\ClearCase\bin;C:\Program Files\Rational\common;C:\Program Files\AccuRev\bin

rem ***Check if PERL_HOME is already defined ***
if exist %PERL_HOME% set PERL_HOME_PATH=%PERL_HOME%

rem *** Check if Perl is installed in the host machine ***
if not exist %PERL_HOME_PATH%\perl.exe echo Error :"%PERL_HOME_PATH%\perl.exe Not found. Either install perl at 'C:\Perl\bin' or Set PERL_HOME to the Perl bin Directory."
if exist %PERL_HOME_PATH%\perl.exe set PATH=%PERL_HOME_PATH%;%PATH%


echo ******************************************
echo Environment is ready if no errors reported
echo ******************************************


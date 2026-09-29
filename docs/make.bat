@ECHO OFF

pushd %~dp0

REM Command file for Sphinx documentation, extended to run Doxygen first.

if "%SPHINXBUILD%" == "" (
	set SPHINXBUILD=sphinx-build
)
set SOURCEDIR=.
set BUILDDIR=_build

if "%1" == "" goto help
if "%1" == "clean" goto clean

where doxygen >nul 2>nul
if errorlevel 1 (
	echo.
	echo.The 'doxygen' command was not found on PATH. Install Doxygen from
	echo.https://www.doxygen.nl/download.html and try again.
	exit /b 1
)
doxygen Doxyfile
if errorlevel 1 exit /b 1

%SPHINXBUILD% -M %1 %SOURCEDIR% %BUILDDIR% %SPHINXOPTS% %O%
goto end

:clean
rmdir /s /q "%BUILDDIR%" 2>nul
rmdir /s /q doxygen 2>nul
goto end

:help
%SPHINXBUILD% -M help %SOURCEDIR% %BUILDDIR% %SPHINXOPTS% %O%

:end
popd

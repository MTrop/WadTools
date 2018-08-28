@echo off
setlocal
REM ==============================================================
REM ===== Detect Tools
REM ==============================================================
where /q gcc
if not %ERRORLEVEL% == 0 goto _nogcc
where /q tr
if not %ERRORLEVEL% == 0 goto _notr
goto __gottools

:__nogcc
echo Could not find GCC on path. Aborting.
goto _end
:__notr
echo Could not find TR on path. Aborting.
goto _end


:__gottools
REM ==============================================================
REM ===== Set Paths
REM ==============================================================
set BUILDDIR=.\build
set DISTDIR=.\dist
set SRCDIR=.\src
REM ==============================================================


REM ==============================================================
REM ===== Check Args
REM ==============================================================

if "%1"=="clean" goto _clean
if "%1"=="packageall" goto _buildpackageall
if "%1"=="package" goto _buildpackage
if "%1"=="incr" goto _buildfileincr
if "%1"=="all" goto _buildall
if exist "%SRCDIR%\%1.c" goto _buildfile

:__helpme
echo build clean 
echo build packageall 
echo build package [packagename] 
echo build incr [filename] 
echo build [filename]
goto _end


:__mkbuildlib
call :__mkbuild
if not exist "%BUILDDIR%\lib" mkdir "%BUILDDIR%\lib"
goto _end

:__mkdist
if not exist "%DISTDIR%" mkdir "%DISTDIR%"
goto _end


:__mkbuild
if not exist "%BUILDDIR%" mkdir "%BUILDDIR%"
goto _end


:__mkbuildpackage
call :__mkbuild
if not exist "%BUILDDIR%\obj\%PACKAGENAME%" mkdir "%BUILDDIR%\obj\%PACKAGENAME%"
goto _end


REM === PACKAGENAME must be set.
:__compilepackage
call :__mkbuildpackage
del /q "%BUILDDIR%\obj\%PACKAGENAME%.nfo"
del /s/q "%BUILDDIR%\obj\%PACKAGENAME%\*.o"
for %%F in ("%SRCDIR%\%PACKAGENAME%\*.c") do (
	gcc -c "%%F" -o "%BUILDDIR%\obj\%PACKAGENAME%\%%~nF.o"
	if not %ERRORLEVEL% == 0 goto _end
	echo Built "%%F" to "%BUILDDIR%\obj\%PACKAGENAME%\%%~nF.o"
)
echo This file is to denote that a package was compiled. > "%BUILDDIR%\obj\%PACKAGENAME%.nfo"
goto _end


REM === MAINFILE must be set.
:__compilefile
for %%F in ("%SRCDIR%\%MAINFILE%.c") do (
	echo %%F > "%BUILDDIR%\%MAINFILE%.args"
	echo -o >> "%BUILDDIR%\%MAINFILE%.args"
	echo "%DISTDIR%\%%~nF.exe" >> "%BUILDDIR%\%MAINFILE%.args"
)
echo "-I%SRCDIR%" >> "%BUILDDIR%\%MAINFILE%.args"

echo Made "%BUILDDIR%\%MAINFILE%.args"

for %%F in ("%BUILDDIR%\obj\*.nfo") do (
	for %%G in ("%BUILDDIR%\obj\%%~nF\*.o") do echo %%G >> "%BUILDDIR%\%MAINFILE%.args"
)

tr "\\" "/" < .\build\%MAINFILE%.args > .\build\%MAINFILE%.args2
del "%BUILDDIR%\%MAINFILE%.args"
ren "%BUILDDIR%\%MAINFILE%.args2" %MAINFILE%.args
gcc @"%BUILDDIR%\%MAINFILE%.args"
if not %ERRORLEVEL% == 0 goto _end
echo Linked to "%DISTDIR%\%MAINFILE%.exe"
echo Done.
goto _end


:_buildpackageall
for /d %%D in (%SRCDIR%\*) do (
	set PACKAGENAME=%%~nD
	call :__compilepackage
)
echo Done.
goto _end


:_buildpackage
set PACKAGENAME=%2
call :__compilepackage
goto _end


:_buildfileincr
call :__mkdist
set MAINFILE=%2
call :__compilefile
goto _end


:_buildfile
call :_buildpackageall
call :__mkdist
set MAINFILE=%1
call :__compilefile
goto _end


:_buildall
call :_buildpackageall
call :__mkdist
for %%F in (%SRCDIR%\*.c) do (
	set MAINFILE=%%~nF
	call :__compilefile
)
goto _end


:_clean
rd /s/q build
rd /s/q dist
echo Done.
goto _end


:_end
exit /b 0

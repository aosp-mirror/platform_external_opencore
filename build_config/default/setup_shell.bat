@echo off

rem *** Set the BASE_DIR ***
set BASE_DIR=
for /f "tokens=* delims=\" %%P in ('cd') do (
	set mypath=%%P
	)
set array=%mypath:\= %
	
for %%E in (%array%) do (
	if .%%E==. goto getout
  if %%E==build_config goto getout
  call set BASE_DIR=%%BASE_DIR%%\%%E
)

:getout 
	set BASE_DIR=%BASE_DIR:~1%

echo Set BASE_DIR to %BASE_DIR% ...


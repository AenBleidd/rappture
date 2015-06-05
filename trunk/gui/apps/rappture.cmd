@REM %~dp0     = extract directory and path of argument 0 (command executed)
@REM %M:~1,-1  = extract every between first and last charcter (the quotes)
@REM These constructs require extended command options (Windows 2000 or later)
@set M="%~dp0"
@wish %M:~1,-1%driver %*

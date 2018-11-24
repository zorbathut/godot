call scons p=windows tools=yes module_mono_enabled=yes mono_glue=no -j12
if %errorlevel% neq 0 exit /b %errorlevel%

.\bin\godot.windows.tools.64.mono.exe --generate-mono-glue modules/mono/glue
if %errorlevel% neq 0 exit /b %errorlevel%

call scons p=windows module_mono_enabled=yes -j12
if %errorlevel% neq 0 exit /b %errorlevel%

.\bin\godot.windows.tools.64.mono.exe c:\werk\shadebinder\project.godot

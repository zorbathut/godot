call scons p=windows tools=yes module_mono_enabled=yes mono_glue=no -j12
start /w .\bin\godot.windows.tools.64.mono.exe --generate-mono-glue modules/mono/glue
call scons p=windows module_mono_enabled=yes -j12
.\bin\godot.windows.tools.64.mono.exe c:\werk\shadebinder\project.godot

@echo off
setlocal ENABLEDELAYEDEXPANSION

call "vcvarsall.bat" x86

:compile
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
cl main.cpp ^
styleblit\styleblit.cpp ^
glfw3\src\context.c ^
glfw3\src\init.c ^
glfw3\src\input.c ^
glfw3\src\monitor.c ^
glfw3\src\vulkan.c ^
glfw3\src\osmesa_context.c ^
glfw3\src\egl_context.c ^
glfw3\src\wgl_context.c ^
glfw3\src\win32_init.c ^
glfw3\src\win32_joystick.c ^
glfw3\src\win32_monitor.c ^
glfw3\src\win32_thread.c ^
glfw3\src\win32_time.c ^
glfw3\src\win32_window.c ^
glfw3\src\window.c ^
glew\src\glew.c ^
/I"." ^
/I"styleblit" ^
/I"glfw3\include" ^
/I"glew\include" ^
/D_GLFW_WIN32 ^
/DGLEW_STATIC ^
user32.lib ^
kernel32.lib ^
shell32.lib ^
gdi32.lib ^
ole32.lib ^
comdlg32.lib ^
opengl32.lib ^
/DNDEBUG ^
/O2 ^
/EHsc ^
/Fe"styleblit.exe" ^
/nologo || goto error
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
goto :EOF

:error
echo FAILED
@%COMSPEC% /C exit 1 >nul

@echo off
cd /d "%~dp0"

"D:\VulkanSDK\1.4.309.0\Bin\glslc.exe" shader.vert -o vert.spv
if errorlevel 1 goto end

"D:\VulkanSDK\1.4.309.0\Bin\glslc.exe" shader.frag -o frag.spv

:end
pause
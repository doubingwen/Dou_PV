# Dependencies

GLFW 3.4 official Windows precompiled binaries (Visual C++ 2022):
https://github.com/glfw/glfw/releases/tag/3.4

GLM 1.0.1 header-only:
https://github.com/g-truc/glm/releases/tag/1.0.1

Layout:
- glfw/include/GLFW: headers
- glfw/lib/x64, glfw/lib/Win32: glfw3dll.lib (DLL import libraries)
- glfw/bin/x64, glfw/bin/Win32: glfw3.dll (runtime libraries)
- glm/glm: GLM headers
- Dependencies.props: Visual Studio include/link configuration and automatic DLL copy

Both Debug and Release use the official GLFW DLL; GLFW itself does not need to be rebuilt.
The project automatically copies the matching architecture's DLL beside the executable.
When distributing the executable, include glfw3.dll.

GLFW_DLL selects DLL imports. GLFW_INCLUDE_NONE avoids implicit OpenGL headers.
GLM_FORCE_DEPTH_ZERO_TO_ONE selects Vulkan's depth range; projection Y is handled separately.
Vulkan.props configures Vulkan SDK headers and vulkan-1.lib for x64. It is imported by Dependencies.props. The SDK is external and resolved through VULKAN_SDK. Win32 Vulkan is not configured because 32-bit SDK libraries are not configured. Select x64 for Vulkan exercises. Include <vulkan/vulkan.h> before <GLFW/glfw3.h> when using GLFW Vulkan declarations. Vulkan runtime is provided by the graphics driver; no project copy is needed.
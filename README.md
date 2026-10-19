# Dou_PV

个人 Vulkan 学习记录，使用 C++、GLFW 和 GLM，在 Windows / Visual Studio 环境中逐步实现渲染流程。

## 当前进度

- 创建 GLFW 窗口，并关闭默认 OpenGL 上下文。
- 填写 `VkApplicationInfo` 和 `VkInstanceCreateInfo`。
- 查询并启用 GLFW 所需的 Vulkan 实例扩展。
- 创建、销毁 Vulkan 实例。
- 枚举可用实例层，检查 `VK_LAYER_KHRONOS_validation`。
- Debug 启用验证层，Release 关闭验证层。

当前仍处于初始化阶段：尚未创建窗口表面、交换链或绘制三角形，也尚未注册调试消息回调。

## 环境要求

- Windows 10/11，支持 Vulkan 的显卡和驱动。
- Visual Studio 2026，安装“使用 C++ 的桌面开发”、MSVC **v145** 工具集及 Windows SDK。
- Vulkan SDK，安装后确保环境变量 `VULKAN_SDK` 指向 SDK 根目录。配置使用 `Include` 和 `Lib`。
- Debug 运行需要可用的 `VK_LAYER_KHRONOS_validation`，通常由 Vulkan SDK 提供。

当前 Vulkan 配置仅支持 **x64**。解决方案保留了 Win32 配置，但未配置其 Vulkan 库，请选择 x64。

## 构建与运行

1. 克隆仓库。
2. 安装上述开发环境；安装 SDK 后，重新启动 Visual Studio 以加载环境变量。
3. 打开根目录的 `Dou_PV.slnx`。
4. 选择 `Debug | x64` 或 `Release | x64`，生成并运行。

也可以在 Visual Studio Developer PowerShell 中构建：

```powershell
msbuild Dou_PV.slnx /p:Configuration=Debug /p:Platform=x64
```

程序位于 `bin/x64/Debug/Dou_PV.exe`。GLFW DLL 会在构建时自动复制到程序旁边。
如果 SDK 不通过环境变量指定，也可以在构建时传入 `/p:DouVulkanSdkRoot="你的 SDK 根目录"`。

## 目录结构

```text
Dou_PV/
├─ Dou_PV.slnx               解决方案
├─ Dou_PV.vcxproj            项目配置
├─ src/main.cpp              学习代码
├─ docs/learning-log.md      学习进度记录
├─ ThirdParty/
│  ├─ glfw/                  GLFW 3.4 官方 Windows 预编译依赖
│  ├─ glm/                   GLM 1.0.1 头文件
│  ├─ Dependencies.props     GLFW、GLM 和 DLL 复制配置
│  └─ Vulkan.props           通过 VULKAN_SDK 引用本机 SDK
├─ bin/                      生成的程序，不提交
└─ obj/                      编译中间文件，不提交
```

## 依赖

- [GLFW 3.4](https://github.com/glfw/glfw/releases/tag/3.4)：使用官方 Visual C++ 2022 预编译 DLL 和导入库，已随仓库提供。
- [GLM 1.0.1](https://github.com/g-truc/glm/releases/tag/1.0.1)：纯头文件库，已随仓库提供。
- [Vulkan SDK](https://vulkan.lunarg.com/)：在本机安装，不随仓库上传。

第三方许可证保留于 `ThirdParty/glfw/LICENSE.md` 和 `ThirdParty/glm/copying.txt`。

## 学习参考

- [Vulkan 中文教程](https://docs.vulkan.net.cn/tutorial/latest/01_Overview.html)
- [Khronos Vulkan Tutorial](https://docs.vulkan.org/tutorial/latest/)

后续每完成一个学习阶段提交一次，便于通过 Git 历史回顾代码变化。

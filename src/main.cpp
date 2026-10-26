#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <glm/glm.hpp>
#include <iostream>
#include<stdexcept>
#include<cstdlib>
#include<vector>
#include<optional>
#include<set>
#include<limits>
#include<algorithm>
#include "../tools/readfile.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
//验证层，目前只使用了一个验证层，后续可以添加更多的验证层。可用层
const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};
//拓展功能
const std::vector<const char*>deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif
//创建调试消息接收器
//自己写的拓展函数 ，调用的是Vulkan的拓展函数vkCreateDebugUtilsMessengerEXT，返回值是VkResult类型
VkResult CreateDebugUtilsMessengerEXT(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	//vkCreateDebugUtilsMessengerEXT函数没有进行链接进来的拓展函数，所有我们要动态获取函数地址然后使用
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance,
		"vkCreateDebugUtilsMessengerEXT");
	if(func!=nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else
	{
		//返回所需扩展不存在
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}
//销毁调试消息接收器
void DestroyDebugUtilsMessengerEXT(
	VkInstance instance, 
	VkDebugUtilsMessengerEXT debugMessenger, 
	const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance,
		"vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, debugMessenger, pAllocator);
	}
}

struct QueueFamilyIndices
{
	/*队列有绘图不一定有呈现*/
	//绘图队列族索引
	std::optional<uint32_t> graphicsFamily;
	//呈现队列族索引
	std::optional<uint32_t> presentFamily;

	bool isComplete() {
		return graphicsFamily.has_value()&&presentFamily.has_value();
	}
};
//交换链的功能
struct SwapChainSupportDetails
{
	//交换链支持多少张图形，尺寸，表面指定的尺寸
	VkSurfaceCapabilitiesKHR capabilities;
	//交换链支持的像素格式和颜色空间
	std::vector<VkSurfaceFormatKHR>formats;
	//交换链支持的呈现模式（按照顺序，新的优先等等）
	std::vector<VkPresentModeKHR>presentModes;
};

class Dou_Application {
public:
	void run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}
private:
	void initWindow() {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API,GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		window = glfwCreateWindow(WIDTH,HEIGHT,"Dou_Vulkan",nullptr,nullptr);
	}
	void initVulkan() {
		createInstance();
		setupDebugMessenger();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
		createImageViews();
		createRenderPass();
		createGraphicsPipline();
		createFramebuffers();
		createCommandPool();
		createCommandBuffer();
		createSyncObjects();
	}
	void mainLoop() {
		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();
			drawFrame();
		}
		vkDeviceWaitIdle(device);
	}
	void cleanup() {
		vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
		vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
		vkDestroyFence(device, inFlightFence, nullptr);
		vkDestroyCommandPool(device, commandPool, nullptr);
		for (auto framebuffer : swapChainFramebuffers)
		{
			vkDestroyFramebuffer(device,framebuffer,nullptr);
		}
		vkDestroyPipeline(device, graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyRenderPass(device, renderPass, nullptr);
		for (auto imageView : swapChainImageViews)
		{
			vkDestroyImageView(device,imageView,nullptr);
		}
		vkDestroySwapchainKHR(device, swapChain, nullptr);
		vkDestroyDevice(device, nullptr);
		if (enableValidationLayers)
		{
			DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}
		vkDestroySurfaceKHR(instance,surface,nullptr);
		vkDestroyInstance(instance, nullptr);
		glfwDestroyWindow(window);
		glfwTerminate();
	}
	void createInstance() {
		if (enableValidationLayers && !checkValidationLayerSupport())
		{
			throw std::runtime_error("validation layers requested, but not available!");
		}
		//应用信息
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Dou_Vulkan";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		//创建信息
		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		//准备拓展列表
		auto extensions=getRequiredExtensions();

		//启用拓展
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();


		//配置调试消息接收器的创建信息
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		//启用验证层
		if (enableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();

			populateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
			
		}
		else {
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
			throw std::runtime_error("failed to create instance!");
		}
	}
	bool checkValidationLayerSupport()
	{
		uint32_t layerCount;
		//获取可用层的数量
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		//VkLayerProperties 是一个结构体，包含了层的名称、规范版本、实现版本和描述信息
		//用上述查到的层数量来创建一个VkLayerProperties的向量来存储可用层的信息
		std::vector<VkLayerProperties> availableLayers(layerCount);
		//这次获取数量和层信息
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		//查看是否所有的验证层都在可用层中
		for (const char* layerName : validationLayers)
		{
			bool layerFound = false;
			for (const auto& layerProperties : availableLayers)
			{
				if (strcmp(layerName, layerProperties.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}
			if (!layerFound)
			{
				return false;
			}
		}
		return true;
	}

	std::vector<const char*>getRequiredExtensions()
	{
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
		//如果启用了验证层，则添加调试扩展
		if (enableValidationLayers)
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		return extensions;
	}
	
	//回调函数，用于处理验证层的消息
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
		VkDebugUtilsMessageTypeFlagsEXT messageType, 
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, 
		void* pUserData)
	{
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
		return VK_FALSE;
	}

	void setupDebugMessenger()
	{
		/*
		你调用 Vulkan API
				↓
		验证层检查，发现问题
				↓
		产生一条验证消息
				↓
		消息接收器确认：符合你设置的消息条件
				↓
		调用你的 debugCallback，并把消息作为参数传进去
				↓
		执行 std::cerr，打印错误内容
		*/
		if (!enableValidationLayers) return;
		
		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		//配置调试消息接收器的创建信息
		populateDebugMessengerCreateInfo(createInfo);

		if (CreateDebugUtilsMessengerEXT(instance, &createInfo, 
			nullptr, &debugMessenger) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to set up debug messenger!");
		}

	}

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		//填写消息接收器的配置
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType= VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		//收到消息时调用的回调函数
		createInfo.pfnUserCallback = debugCallback;
	}
	
	void pickPhysicalDevice()
	{ 
		uint32_t deviceCount = 0;
		//查询显卡数量
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
		if (deviceCount == 0)
		{
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}
		//将查询到的显卡数量存入一个向量中
		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
		
		for (const auto& device : devices)
		{
			if (isDeviceSuitable(device))
			{
				physicalDevice = device;
				break;
			}
		}

		if(physicalDevice==VK_NULL_HANDLE)
		{
			throw std::runtime_error("failed to find a suitable GPU!");
		}
	}
	//查询显卡是否满足要求
	bool isDeviceSuitable(VkPhysicalDevice device)
	{
		////查询设备 名称。类型，支持Vulkan版本
		//VkPhysicalDeviceProperties deviceProperties;
		//vkGetPhysicalDeviceProperties(device, &deviceProperties);

		////查询对纹理压缩，64位浮点数，多视口渲染，采样器各向异性过滤，线性过滤等的支持情况
		//VkPhysicalDeviceFeatures deviceFeatures;
		//vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		//
		//return deviceProperties.deviceType==VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU&&
		//	deviceFeatures.geometryShader;
		QueueFamilyIndices indices = findQueueFamilies(device);
		//查看是否支持交互链扩展
		bool extensionsSupported = checkDeviceExtensionSupport(device);
		bool swapChainAdequate = false;
		if (extensionsSupported)
		{
			//查看交换链功能是否足够
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
			swapChainAdequate = !swapChainSupport.formats.empty() && 
				!swapChainSupport.presentModes.empty();

		}
		return indices.isComplete()&&extensionsSupported&&swapChainAdequate;
	}
	//查询是否支持交换链扩展
	bool checkDeviceExtensionSupport(VkPhysicalDevice device)
	{
		//我们需要的功能，然后去在GPU上查询是否支持这些功能，如果不支持就不能使用这个GPU
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, 
			&extensionCount, nullptr);

		std::vector<VkExtensionProperties>availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, 
			&extensionCount, availableExtensions.data());

		std::set<std::string>requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		for (const auto& extension : availableExtensions)
		{
			requiredExtensions.erase(extension.extensionName);
		}
		//GPU支持所有我们需要的功能就返回true，否则返回false
		return requiredExtensions.empty();
	}
	//队列族查找
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices;
		
		uint32_t queueFamilyCount = 0;

		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties>queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies)
		{
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				//找到有绘图功能的队列族
				indices.graphicsFamily = i;
			}
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
			if (presentSupport)
			{
				//找到有呈现功能的队列族
				indices.presentFamily = i;
			}

			if (indices.isComplete())
			{
				break;
			}
			i++;
		}
		return indices;
	}

	void createLogicalDevice()
	{
		//获取物理设备的队列族索引
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);



		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t>uniqueQueueFamilies = {indices.graphicsFamily.value()
		,indices.presentFamily.value()};
		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			//把每个队列族的创建信息都放入队列创建信息向量中
			queueCreateInfos.push_back(queueCreateInfo);
		}

		//创建该队列的功能信息，暂时不需要任何功能，所以设置为默认值
		VkPhysicalDeviceFeatures deviceFeatures{};
		//创建逻辑设备信息
		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pEnabledFeatures = &deviceFeatures;
		//拓展功能数量
		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();

		//创建逻辑设备
		if(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create logical device!");
		}
		//从逻辑设备中获取队列句柄
		vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
	}

	void createSurface()
	{
		if (glfwCreateWindowSurface(instance,window,nullptr, &surface) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create window surface!");	
		}
	}
	
	//查询交换链支持的功能
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device)
	{
		SwapChainSupportDetails details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, 
			&details.capabilities);
		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, 
			&formatCount, nullptr);
		if (formatCount != 0)
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device,surface,
				&formatCount,details.formats.data());
		}
		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, 
			&presentModeCount, nullptr);
		if (presentModeCount!= 0)
		{
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, 
				&presentModeCount, details.presentModes.data());	
		}
		return details;

	}
	
	//选择交换链正确设置
	VkSurfaceFormatKHR chooseSwapSurfaceFormat
	(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const auto& availableFormat : availableFormats)
		{
			if(availableFormat.format==VK_FORMAT_B8G8R8A8_SRGB
				&&availableFormat.colorSpace== VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				return availableFormat;
		}
		return availableFormats[0];
	}
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> & availablePresentModes)
	{
		for (const auto& availablePresentMode : availablePresentModes)
		{
			return availablePresentMode;
		}
		return VK_PRESENT_MODE_FIFO_KHR;
	}
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width 
			!= std::numeric_limits<uint32_t>::max())
		{
			//如果指定了就有默认的尺寸，直接返回
			return capabilities.currentExtent;
		}
		else
		{
			int width,height;
			glfwGetFramebufferSize(window,&width,&height);

			VkExtent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};
				
			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width,
				capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height,
				capabilities.maxImageExtent.height);
			return actualExtent;
		}
	}
	
	//创建交换链
	void createSwapChain()
	{
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

		//限制交换链图像数量
		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
		if (swapChainSupport.capabilities.maxImageCount > 0
			&& imageCount > swapChainSupport.capabilities.maxImageCount)
		{
			imageCount = swapChainSupport.capabilities.maxImageCount;	
		}

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		//交换链图像的用途，这里是作为颜色附件使用
		createInfo.imageUsage= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(),indices.presentFamily.value() };
		if (indices.graphicsFamily != indices.presentFamily)
		{	//当渲染和呈现不在一个队列时
			//允许队列族共享这个交互链图像
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			//共享的队列族数量
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			//独占模式
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
		}
		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;

		createInfo.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create swap chain!");
		}

		//创建完交互链后，拿到交换链中的图像

		//拿到数量
		vkGetSwapchainImagesKHR(device,swapChain,&imageCount,nullptr);
		//根据数量设置大小
		swapChainImages.resize(imageCount);
		//将信息读到变量组中
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;
	}
	
	//创建imageviews 
	void createImageViews()
	{
		swapChainImageViews.resize(swapChainImages.size());
		for (size_t i = 0;i < swapChainImages.size();i++)
		{
			VkImageViewCreateInfo createInfo{};
			createInfo.sType= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = swapChainImages[i];
			createInfo.viewType= VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = swapChainImageFormat;
			//原本RGBA的颜色值，可以设置通道映射
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			//描述了图像的用途以及应访问图像的哪一部分，我们的图像将用作颜色目标，没有任何 mipmap 级别或多个图层。

			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(device, &createInfo, nullptr, 
				&swapChainImageViews[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create image views!");
			}
		}


	}
	
	//创建shader 代码模块(作用提供shader代码)
	VkShaderModule createShaderModule(const std::vector<char>& code)
	{
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode= reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule)!=VK_SUCCESS)
		{
			throw std::runtime_error("failed to create shader module");
		}
		return shaderModule;
	}
	//图形管线创建
	void createGraphicsPipline()
	{
		
		auto vertShaderCode = readFile("src/shaders/vert.spv");
		auto fragShaderCode = readFile("src/shaders/frag.spv");
		VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);


		//创建着色器

		//顶点着色器信息
		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main"; //从着色代码的main函数进入
		//片段着色器信息
		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		//指定管线中哪些状态由命令动态设置，而不是在创建管线时固定
		//指定视口变换和裁剪是动态改变的
		std::vector<VkDynamicState>dynamicStates = {
			 VK_DYNAMIC_STATE_VIEWPORT,
			 VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType= VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount= static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		//是“顶点输入配置”结构体，告诉管线如何从顶点缓冲区读取数据。
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional
		
		//是“图元装配配置”，决定把顶点按什么规则组成点、线或三角形。
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		//视口变换设置（透视投影以后） NDC{-1，1}映射到{0，高or宽}
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swapChainExtent.width;
		viewport.height = (float)swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		//裁剪
		VkRect2D scissor{};
		scissor.offset = { 0,0 };
		scissor.extent = swapChainExtent;

		//vulkan支持多个视口和裁剪
		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;


		//多重采样信息创建
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f; // Optional
		multisampling.pSampleMask = nullptr; // Optional
		multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
		multisampling.alphaToOneEnable = VK_FALSE; // Optional

		//光栅化信息创建
		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;//剔除模式开启
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;//正面面的顺序
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f; // Optional
		rasterizer.depthBiasClamp = 0.0f; // Optional
		rasterizer.depthBiasSlopeFactor = 0.0f; // Optional
		rasterizer.lineWidth = 1.0f; // 

		//深度和模板测试创建

		//单一个颜色附件的设置
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | 
			VK_COLOR_COMPONENT_G_BIT | 
			VK_COLOR_COMPONENT_B_BIT | 
			VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
		//引用这些设置的“总配置”，为所有帧缓存设置
		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f; // Optional
		colorBlending.blendConstants[1] = 0.0f; // Optional
		colorBlending.blendConstants[2] = 0.0f; // Optional
		colorBlending.blendConstants[3] = 0.0f; // Optional


		//管线布局信息创建 程序通过哪些资源接口，把数据提供给着色器。
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0; // Optional
		pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
		pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
		pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}


		//管线信息配置
		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = nullptr; // Optional
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional

		//管线创建
		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics pipeline!");
		}
		//销毁shaderModule
		vkDestroyShaderModule(device, fragShaderModule, nullptr);
		vkDestroyShaderModule(device, vertShaderModule, nullptr);


	}

	void createRenderPass()
	{
		//附件对于pass来言如何使用
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = swapChainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		
		//pass使用哪个附件
		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		
		//子pass，绑定使用的附件
		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		//给渲染通道外部的操作与内部的子通道 
		// 0 建立依赖，让子通道 0 按正确顺序开始使用附件。
		//顺序规定 先交换链图像可用 再转化图像布局 再子通道渲染
		//规定子通道在哪个位置运行
		
		//让图像可用在 renderpass之前进行
		VkSubpassDependency dependency{};
		//这条依赖约束哪些管线阶段
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		//源 的同步范围
		//颜色附件输出阶段
		//来源侧不指定需要同步的内存访问类型
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		//目的的同步范围
		//子通道 0 中，清除、写入颜色附件的相关操作，要遵守这条等待关系。
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		//这条依赖在目标侧要同步的内存访问，是“写入颜色附件”。
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;


		//主pass设置
		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render pass!");
		}
	}
	
	//创建帧缓冲
	void createFramebuffers()
	{	//为每张交换链图像视图，创建一个对应的帧缓冲。
		swapChainFramebuffers.resize(swapChainImageViews.size());
		for (size_t i = 0;i < swapChainImageViews.size();i++)
		{
			VkImageView attachments[] = { swapChainImageViews[i] };
			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create framebuffer!");
			}
		}
	}
	
	//创建命令缓冲池
	void createCommandPool()
	{
		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
	
		if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create command pool!");
		}
	}
	void createCommandBuffer()
	{
		//从 commandPool 中分配一个主命令缓冲区，并把句柄保存到 commandBuffer。

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;
		if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}
	}

	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
		//将执行的命令写入commandbuffer中，开始录入指令
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0; // Optional
		beginInfo.pInheritanceInfo = nullptr; // Optional

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		//指定这次录制的命令使用的时哪个渲染通道
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapChainExtent;
		VkClearValue clearColor = { { { 0.0f, 0.0f, 0.0f, 1.0f } } };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;
		//指令开始录入
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		//管线绑定
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
		//我们指定可以动态调整的数据
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(swapChainExtent.width);
		viewport.height = static_cast<float>(swapChainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = swapChainExtent;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
		vkCmdDraw(commandBuffer, 3, 1, 0, 0);
		vkCmdEndRenderPass(commandBuffer);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}

	}
	
	void drawFrame()
	{
		//上一帧的GPU工作是否完成，不完成就在这儿等待
		vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
		// 等待成功后，CPU 才会执行这里
		vkResetFences(device, 1, &inFlightFence);


		//获取交换链图像
		uint32_t imageIndex;
		vkAcquireNextImageKHR(device, swapChain, 
			UINT64_MAX, imageAvailableSemaphore,
			VK_NULL_HANDLE, &imageIndex);

		//命令清除
		vkResetCommandBuffer(commandBuffer, 0);
		recordCommandBuffer(commandBuffer, imageIndex);

		//提交命令给缓冲区
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
		//等待的阶段选择在颜色附件输出阶段
		//信号量是交换链的图像可以写，当颜色附件可以输出来等待这个信号量
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		//规定本次图形提交的颜色附件输出阶段，要等待图像可用信号。
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		//本次图形提交执行完成后，让 renderFinishedSemaphore 发出信号。
		VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr; // Optional
		vkQueuePresentKHR(presentQueue, &presentInfo);
	}

	//创建同步对象
	void createSyncObjects()
	{
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS ||
			vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS) {
			throw std::runtime_error("failed to create semaphores!");
		}

	}
	
	GLFWwindow * window;
	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	//逻辑设备
	VkDevice device;
	//物理设备
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

	//绘图队列和呈现队列
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	//Vulkan呈现的目标，将这个surface和GLFW的窗口绑定在一起，GLFW会自动选择合适的呈现方式
	VkSurfaceKHR surface;

	//交换链
	VkSwapchainKHR swapChain;
	//交换链中的图像
	std::vector<VkImage> swapChainImages;
	//交换链中图像格式
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	//VkImageView保存如何使用这个交互链中的图像
	std::vector<VkImageView> swapChainImageViews;

	//pass
	VkRenderPass renderPass;
	//pass输入设置
	VkPipelineLayout pipelineLayout;

	//管线
	VkPipeline graphicsPipeline;

	//帧缓冲
	//VkFramebuffer提供具体的附件视图
	std::vector<VkFramebuffer>swapChainFramebuffers;
	//命令缓冲池
	VkCommandPool commandPool;
	//命令缓冲
	VkCommandBuffer commandBuffer;

	// 信号量和栅栏

	VkSemaphore imageAvailableSemaphore; //交换链图像可用
	VkSemaphore renderFinishedSemaphore; //绘制完成，可用呈现
	VkFence inFlightFence; //命令提交完成，cpu准备复用命令缓冲区
};
int main() {
	Dou_Application app;
	try {
		app.run();
	} catch (const std::exception&e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE; 
	}
	return EXIT_SUCCESS;
}





/*验证层
① checkValidationLayerSupport()
   检查所需验证层是否存在
		↓
② getRequiredExtensions()
   把 VK_EXT_debug_utils 加入扩展清单
		↓
③ createInstance() 中填写配置
   启用验证层、扩展
   调用 populateDebugMessengerCreateInfo()
   将回调配置挂到 pNext
		↓
④ vkCreateInstance()
   创建实例，验证层开始参与检查
		↓
⑤ setupDebugMessenger()
   再调用 populateDebugMessengerCreateInfo()
   调用 CreateDebugUtilsMessengerEXT()
   创建显式的消息接收器
		↓
⑥ 产生符合条件的消息时
   自动调用 debugCallback()，打印消息
		↓
⑦ 退出时
   DestroyDebugUtilsMessengerEXT()
   然后才能 vkDestroyInstance()
  */
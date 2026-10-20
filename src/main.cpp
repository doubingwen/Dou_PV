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
	}
	void mainLoop() {
		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();
		}
	}
	void cleanup() {
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
			//可以设置对那些通道进行混合
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
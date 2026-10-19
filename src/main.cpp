#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include<stdexcept>
#include<cstdlib>
#include<vector>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
//验证层，目前只使用了一个验证层，后续可以添加更多的验证层。可用层
const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
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
	}
	void mainLoop() {
		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();
		}
	}
	void cleanup() {
		if (enableValidationLayers)
		{
			DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}
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
	GLFWwindow* window;
	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
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
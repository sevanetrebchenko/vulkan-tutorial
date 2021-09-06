
// Standard includes.
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <set>

#include "application.h"

namespace VT {

	Application::Application(int width, int height) : width_(width),
													  height_(height)
													  {
	}

	Application::~Application() {
	}

	void Application::Run() {
		Initialize();
		Update();
		Shutdown();
	}

	VkBool32 Application::DebugMessageCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData) {
		if (severity > VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
			std::cerr << callbackData->pMessage << std::endl;
		}

		return VK_FALSE; // Returns true if the function call that triggered the layer should be aborted. Normally used to test the layers themselves.
	}

	void Application::Initialize() {
		InitializeGLFW();
		InitializeVKInstance();
		InitializeDebugMessenger();
		InitializeVulkanSurface();
		InitializePhysicalDevice();
		InitializeLogicalDevice();
	}

	void Application::Update() {
		while (!glfwWindowShouldClose(window_)) {
			glfwPollEvents();
		}
	}

	void Application::Shutdown() {
		if (enableValidationLayers_) {
			auto function = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance_, "vkDestroyDebugUtilsMessengerEXT");
			if (!function) {
				throw std::runtime_error("Failed to find function for destroying debug messenger.");
			}

			function(instance_, messenger_, nullptr);
		}

		// Functions take optional callbacks.
		vkDestroyDevice(logicalDevice_, nullptr);

		vkDestroySurfaceKHR(instance_, surface_, nullptr); // Surface needs to be destroyed before instance.
		vkDestroyInstance(instance_, nullptr);

		glfwDestroyWindow(window_);
		glfwTerminate();
	}

	void Application::InitializeGLFW() {
		// Initialize GLFW.
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window_ = glfwCreateWindow(width_, height_, "Vulkan Tutorial", nullptr, nullptr);
	}

	void Application::InitializeVKInstance() {
		// Create Vulkan instance.
		VkApplicationInfo appInfo { };
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Vulkan Tutorial";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "N/A";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_2;

		// Tells the graphics driver which global extensions and validation layers to use.
		// Global (applies to the entire program).
		VkInstanceCreateInfo createInfo { };
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		// Get supported extensions.
		std::vector<VkExtensionProperties> supportedExtensions;
		GetSupportedExtensions(supportedExtensions);

		// Get desired extensions.
		std::vector<const char*> desiredExtensions;
		GetDesiredInstanceExtensions(desiredExtensions);

		if (!CheckInstanceExtensions(supportedExtensions, desiredExtensions)) {
			throw std::runtime_error("Requested extensions not supported.");
		}

		createInfo.enabledExtensionCount = desiredExtensions.size();
		createInfo.ppEnabledExtensionNames = desiredExtensions.data();

		// Get supported validation layers.
		std::vector<const char*> validationLayerNames {
			"VK_LAYER_KHRONOS_validation"
		};

		if (enableValidationLayers_) {
			if (!CheckValidationLayers(validationLayerNames)) {
				throw std::runtime_error("Requested validation layers not available.");
			}

			validationLayerNames_ = validationLayerNames;

			VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo { };
			SetupDebugMessengerUtils(messengerCreateInfo);

			createInfo.enabledLayerCount = validationLayerNames_.size();
			createInfo.ppEnabledLayerNames = validationLayerNames_.data();
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)(&messengerCreateInfo); // Setup debug messaging during Vulkan instance creation.
		}
		else {
			createInfo.enabledLayerCount = 0; // Don't use any validation layers.
			createInfo.pNext = nullptr; // Don't log debug messages during instance creation.
		}

		// Create Vulkan instance.
		if (vkCreateInstance(&createInfo, nullptr, &instance_) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Vulkan instance.");
		}
	}

	void Application::InitializeDebugMessenger() {
		if (!enableValidationLayers_) {
			return;
		}

		VkDebugUtilsMessengerCreateInfoEXT createInfo { };
		SetupDebugMessengerUtils(createInfo);

		auto function = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance_, "vkCreateDebugUtilsMessengerEXT");
		if (!function) {
			throw std::runtime_error("Failed to find function to create debug messenger.");
		}

		bool result = function(instance_, &createInfo, nullptr, &messenger_);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to create debug messenger.");
		}
	}

	void Application::InitializePhysicalDevice() {
		std::vector<VkPhysicalDevice> physicalDevices;
		GetSupportedPhysicalDevices(physicalDevices);

		// Check found devices for compatibility.
		if (!CheckPhysicalDevices(physicalDevices)) {
			throw std::runtime_error("Failed to find compatible physical device.");
		}
	}

	void Application::InitializeLogicalDevice() {
		// Queues are something you submit command buffers to.
		// Create structure to hold data for all the desired queue families.
		std::set<unsigned> uniqueQueueFamilies = {
			physicalDeviceData_.queueIndices_.graphicsFamily_.value(),
		    physicalDeviceData_.queueIndices_.presentationFamily_.value()
		};
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(uniqueQueueFamilies.size());

		float queuePriority = 1.0f;
		for (unsigned queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo { };
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;

			queueCreateInfos.push_back(queueCreateInfo);
		}

		// Create logical device.
		VkDeviceCreateInfo createInfo { };
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = queueCreateInfos.size();
		createInfo.pQueueCreateInfos = queueCreateInfos.data();

		createInfo.pEnabledFeatures = &physicalDeviceData_.deviceFeatures;

		// Enable swapchain.
		createInfo.ppEnabledExtensionNames = physicalDeviceData_.deviceExtensions_.data();
		createInfo.enabledExtensionCount = physicalDeviceData_.deviceExtensions_.size();

		if (enableValidationLayers_) {
			// No distinction between device and instance validation layer specification on up to date Vulkan applications.
			// Specified for compatibility.
			createInfo.enabledLayerCount = validationLayerNames_.size();
			createInfo.ppEnabledLayerNames = validationLayerNames_.data();
		}
		else {
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateDevice(physicalDeviceData_.physicalDevice_, &createInfo, nullptr, &logicalDevice_) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create logical device.");
		}

		// Since only one queue of each type is being used, index is 0.
		unsigned graphicsQueueIndex = 0;
		unsigned presentationQueueIndex = 0;

		// Query the location of the graphics queue to use for rendering.
		vkGetDeviceQueue(logicalDevice_, physicalDeviceData_.queueIndices_.graphicsFamily_.value(), graphicsQueueIndex, &graphicsQueue_);

		// Query the location of the presentation queue to be able to present images to the surface.
		vkGetDeviceQueue(logicalDevice_, physicalDeviceData_.queueIndices_.presentationFamily_.value(), presentationQueueIndex, &presentationQueue_);
	}

	void Application::InitializeVulkanSurface() {
		if (glfwCreateWindowSurface(instance_, window_, nullptr, &surface_) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Vulkan window surface.");
		}
	}

	// Assumes messenger info has been initialized.
	void Application::SetupDebugMessengerUtils(VkDebugUtilsMessengerCreateInfoEXT& messengerInfo) {
		messengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		messengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		messengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		messengerInfo.pfnUserCallback = DebugMessageCallback;
		messengerInfo.pUserData = nullptr; // No user data.
	}

	void Application::GetSupportedExtensions(std::vector<VkExtensionProperties>& extensionData) {
		unsigned extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr); // Get the number of extensions.

		extensionData.resize(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensionData.data()); // Get extension data.
	}

	void Application::GetDesiredInstanceExtensions(std::vector<const char*>& extensions) {
		unsigned glfwExtensionCount = 0;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		extensions = std::vector<const char*>(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (enableValidationLayers_) {
			// Emplace debug logger extension.
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
	}

	void Application::GetDesiredPhysicalDeviceExtensions(std::vector<const char*>& deviceExtensions) {
		deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	}

	void Application::GetSupportedPhysicalDevices(std::vector<VkPhysicalDevice>& deviceData) {
		unsigned numDevices = 0;
		vkEnumeratePhysicalDevices(instance_, &numDevices, nullptr); // Query number of available physical devices.

		if (numDevices <= 0) {
			throw std::runtime_error("No available physical devices for Vulkan to work with.");
		}

		deviceData = std::vector<VkPhysicalDevice>(numDevices);
		vkEnumeratePhysicalDevices(instance_, &numDevices, deviceData.data());
	}

	void Application::GetSupportedQueueFamilies(const VkPhysicalDevice& physicalDevice, std::vector<VkQueueFamilyProperties>& queueData) {
		unsigned numQueueFamilies = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &numQueueFamilies, nullptr);

		queueData = std::vector<VkQueueFamilyProperties>(numQueueFamilies);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &numQueueFamilies, queueData.data());
	}

	void Application::GetSupportedPhysicalDeviceExtensions(const VkPhysicalDevice& physicalDevice, std::vector<VkExtensionProperties>& extensions) {
		unsigned numExtensions = 0;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &numExtensions, nullptr);

		extensions = std::vector<VkExtensionProperties>(numExtensions);
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &numExtensions, extensions.data());
	}

	bool Application::CheckInstanceExtensions(const std::vector<VkExtensionProperties>& supportedExtensions, const std::vector<const char*>& desiredExtensions) {
		// Verify extensions exist with supported extensions.
		bool complete = true;

		for (const char* desiredExtension : desiredExtensions) {
			bool found = false;

			for (const VkExtensionProperties& supportedExtension : supportedExtensions) {
				if (strcmp(desiredExtension, supportedExtension.extensionName) == 0) {
					found = true;
					break;
				}
			}

			if (!found) {
				complete = false;
				std::cout << "Encountered unsupported instance extension: " << desiredExtension << std::endl;
			}
		}

		return complete;
	}

	bool Application::CheckValidationLayers(std::vector<const char*>& validationLayerNames) {
		// Get the number of validation layers
		unsigned numValidationLayers = 0;
		vkEnumerateInstanceLayerProperties(&numValidationLayers, nullptr);

		// Get validation layer objects.
		std::vector<VkLayerProperties> availableValidationLayers(numValidationLayers);
		vkEnumerateInstanceLayerProperties(&numValidationLayers, availableValidationLayers.data());

		bool complete = true;

		// Compare supported validation layers with desired.
		for (const char* desired : validationLayerNames) {
			bool found = false;

			for (const VkLayerProperties& layerProperties : availableValidationLayers) {
				if (strcmp(desired, layerProperties.layerName) == 0) {
					found = true;
					break;
				}
			}

			if (!found) {
				// Not all desired layers are supported.
				complete = false;
				std::cout << "Encountered unsupported validation layer: " << desired << std::endl;
			}
		}

		return complete;
	}

	bool Application::CheckPhysicalDevices(const std::vector<VkPhysicalDevice>& physicalDevices) {
		bool found = false;

		for (const VkPhysicalDevice& physicalDevice : physicalDevices) {
			if (CheckPhysicalDevice(physicalDevice)) {
				found = true;
				break;
			}
		}

		return found;
	}

	bool Application::CheckPhysicalDevice(const VkPhysicalDevice& physicalDevice) {
		VkPhysicalDeviceProperties deviceProperties; // Used for basic device properties, like name, type, and supported Vulkan versions.
		VkPhysicalDeviceFeatures deviceFeatures; // Used for optional features like texture compression, 64-bit floats, and multi vieport rendering.

		// Query device info.
		vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
		vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

		std::cout << "Found physical device: " << deviceProperties.deviceName << std::endl;

		QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(physicalDevice);

		// Device extensions.
		std::vector<const char*> desiredDeviceExtensions;
		GetDesiredPhysicalDeviceExtensions(desiredDeviceExtensions);

		std::vector<VkExtensionProperties> supportedDeviceExtensions;
		GetSupportedPhysicalDeviceExtensions(physicalDevice, supportedDeviceExtensions);

		bool extensionsSupported = CheckPhysicalDeviceExtensionSupport(physicalDevice, supportedDeviceExtensions, desiredDeviceExtensions);

		if (queueFamilyIndices.IsComplete() && extensionsSupported) {
			// Found compatible device.
			physicalDeviceData_.physicalDevice_ = physicalDevice;
			physicalDeviceData_.deviceProperties = deviceProperties;
			physicalDeviceData_.deviceFeatures = deviceFeatures;
			physicalDeviceData_.queueIndices_ = queueFamilyIndices;
			physicalDeviceData_.deviceExtensions_ = desiredDeviceExtensions;

			return true;
		}

		return false;
	}

	bool Application::CheckPhysicalDeviceExtensionSupport(const VkPhysicalDevice& physicalDevice, const std::vector<VkExtensionProperties>& supportedDeviceExtensions, const std::vector<const char*>& desiredDeviceExtensions) {
		// Make sure all desired extensions are present in the device extensions.
		bool complete = true;

		for (const char* desiredExtension : desiredDeviceExtensions) {
			bool found = false;

			for (const VkExtensionProperties& deviceExtension : supportedDeviceExtensions) {
				if (strcmp(deviceExtension.extensionName, desiredExtension) == 0) {
					found = true;
					break;
				}
			}

			if (!found) {
				complete = false;
				std::cout << "Encountered unsupported physical device extension: " << desiredExtension << std::endl;
			}
		}

		return complete;
	}

	// Every operation in Vulkan requires commands to be submitted to a queue.
	// There are different families of queues, each used for a specific purpose.
	// This function checks for the existence of the required queue families and the location of the queue that supports graphical operations - the graphics queue.
	QueueFamilyIndices Application::FindQueueFamilies(const VkPhysicalDevice& physicalDevice) {
		QueueFamilyIndices queueFamilyIndices;

		// VkQueueFamilyProperties contains details about the queue family, including supported operations and maximum number of queues that can be created.
		std::vector<VkQueueFamilyProperties> queueData;
		GetSupportedQueueFamilies(physicalDevice, queueData);

		// Find queue family that supports graphics.
		for (unsigned i = 0; i < queueData.size(); ++i) {
			const VkQueueFamilyProperties& queueProperties = queueData[i];

			if (queueProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				// Found suitable device that supports graphics.
				queueFamilyIndices.graphicsFamily_ = i;

				VkBool32 presentationSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface_, &presentationSupport);

				if (presentationSupport) {
					queueFamilyIndices.presentationFamily_ = i;
					break;
				}
			}
		}

		return queueFamilyIndices;
	}

}

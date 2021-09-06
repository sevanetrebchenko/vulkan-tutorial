
// Standard includes.
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <set>
#include <algorithm>

#include "application.h"

namespace VT {

	Application::Application(int width, int height) : width_(width),
													  height_(height),
													  enableValidationLayers_(true)
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
		InitializeSwapChain();
	}

	void Application::Update() {
		while (!glfwWindowShouldClose(window_)) {
			glfwPollEvents();
		}
	}

	void Application::Shutdown() {
		// Functions take optional callbacks.
		vkDestroySwapchainKHR(logicalDevice_, swapChain_, nullptr); // Needs to be destroyed before the logical device.
		vkDestroyDevice(logicalDevice_, nullptr);

		if (enableValidationLayers_) {
		    auto function = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance_, "vkDestroyDebugUtilsMessengerEXT");
		    if (!function) {
		        throw std::runtime_error("Failed to find function for destroying debug messenger.");
		    }

		    function(instance_, messenger_, nullptr);
		}

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

		if (supportedExtensions.empty()) {
		    throw std::runtime_error("No supported Vulkan extensions.");
		}

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
		QueueFamilyIndexData& indexData = physicalDeviceData_.queueIndices_;
		std::set<unsigned> uniqueQueueFamilies = {
            indexData.graphicsFamily_.value(),
            indexData.presentationFamily_.value()
		};
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

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

	void Application::InitializeSwapChain() {
	    swapChainData_ = QuerySwapChainSupport(physicalDeviceData_.physicalDevice_);

	    VkSurfaceFormatKHR surfaceFormat = ChooseSwapChainSurfaceFormat(swapChainData_.formats_);
	    VkPresentModeKHR presentationMode = ChooseSwapChainPresentationMode(swapChainData_.presentationModes_);
	    VkExtent2D extent = ChooseSwapChainExtent(swapChainData_.surfaceCapabilities_);

        // Request 1 more than minimum to prevent waiting for driver to complete operations before acquiring an image to render to.
        unsigned swapChainImageCount = swapChainData_.surfaceCapabilities_.minImageCount + 1;
        swapChainImageCount = std::clamp(swapChainImageCount, swapChainImageCount, swapChainData_.surfaceCapabilities_.maxImageCount); // maxImageCount = 0 means no maximum number of images.

        // Create swapchain.
        VkSwapchainCreateInfoKHR createInfo { };
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface_;

        createInfo.minImageCount = swapChainImageCount; // Implementation may create more images, since only the minimum is specified.
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1; // Specifies number of layers each image consists of.
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // Color render attachment.

        QueueFamilyIndexData& indexData = physicalDeviceData_.queueIndices_; // Guaranteed to exist due to requirements from physical device construction.
        unsigned graphicsFamilyQueueIndex = indexData.graphicsFamily_.value();
        unsigned presentationFamilyQueueIndex = indexData.presentationFamily_.value();

        unsigned queueFamilyIndexData[] = { graphicsFamilyQueueIndex, presentationFamilyQueueIndex };
        if (graphicsFamilyQueueIndex != presentationFamilyQueueIndex) {
            // Separate queues used for graphics and presentation.
            // Drawing images on the graphics queue, submitting images on the presentation queue.
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // Images can be used across multiple queues without explicit ownership transfer.
            // TODO: Look into ownership transferring, as this option offers better performance.

            createInfo.queueFamilyIndexCount = 2; // Number of queue families that have access to swapchain images with VK_SHARING_MODE_CONCURRENT.
            createInfo.pQueueFamilyIndices = queueFamilyIndexData; // Which queue families have access.
        }
        else {
            // Same queue used for garphics and presentation.
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // Images are owned by one family queue at a time an ownership must be transferred.
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        createInfo.preTransform = swapChainData_.surfaceCapabilities_.currentTransform; // Transformation pre-applied to all swapchain images.
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // Blending with other images.
        createInfo.presentMode = presentationMode;
        createInfo.clipped = VK_TRUE; // Color of obscured pixels does not matter.
        createInfo.oldSwapchain = nullptr; // Swapping between different swapchains requires the old one to be stored.

        if (vkCreateSwapchainKHR(logicalDevice_, &createInfo, nullptr, &swapChain_) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create swapchain.");
        }

        // Query the number of created images in the swapchain. Implementation may create more images, since only the minimum is specified.
        unsigned imageCount = 0;
        if (vkGetSwapchainImagesKHR(logicalDevice_, swapChain_, &imageCount, nullptr) != VK_SUCCESS) {
            throw std::runtime_error("Failed to get number of images in swapchain.");
        }
        swapChainImages_.resize(imageCount);

        // Get images.
        if (vkGetSwapchainImagesKHR(logicalDevice_, swapChain_, &imageCount, swapChainImages_.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to get swapchain images.");
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

		if (extensionCount != 0) {
		    extensionData.resize(extensionCount);

		    // Get extension data.
		    if (vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensionData.data()) != VK_SUCCESS) {
		        throw std::runtime_error("Failed to get instance extension properties.");
		    }
		}
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
	    // Push back desire for swap chain.
	    // Swap chain is a queue of images that are waiting to be presented on the screen.
	    // Application grabs an image, renders it, then returns it to the queue.
		deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	}

	void Application::GetSupportedPhysicalDevices(std::vector<VkPhysicalDevice>& deviceData) {
		unsigned numDevices = 0;

		// Query number of available physical devices.
		if (vkEnumeratePhysicalDevices(instance_, &numDevices, nullptr) != VK_SUCCESS) {
		    throw std::runtime_error("Failed to get number of available physical devices.");
		}

		if (numDevices <= 0) {
			throw std::runtime_error("No available physical devices for Vulkan to work with.");
		}

		deviceData = std::vector<VkPhysicalDevice>(numDevices);
		if (vkEnumeratePhysicalDevices(instance_, &numDevices, deviceData.data()) != VK_SUCCESS) {
		    throw std::runtime_error("Failed to get available physical devices.");
		}
	}

	void Application::GetSupportedQueueFamilies(const VkPhysicalDevice& physicalDevice, std::vector<VkQueueFamilyProperties>& queueData) {
		unsigned numQueueFamilies = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &numQueueFamilies, nullptr);

		if (numQueueFamilies != 0) {
		    queueData = std::vector<VkQueueFamilyProperties>(numQueueFamilies);
		    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &numQueueFamilies, queueData.data());
		}
	}

	void Application::GetSupportedPhysicalDeviceExtensions(const VkPhysicalDevice& physicalDevice, std::vector<VkExtensionProperties>& extensions) {
		unsigned numExtensions = 0;
		if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &numExtensions, nullptr) != VK_SUCCESS) {
		    throw std::runtime_error("Failed to get number of logical devices.");
		}

		if (numExtensions != 0) {
		    extensions = std::vector<VkExtensionProperties>(numExtensions);
		    if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &numExtensions, extensions.data()) != VK_SUCCESS) {
		        throw std::runtime_error("Failed to get logical device extension properties.");
		    }
		}
	}

	void Application::GetSupportedSurfaceFormats(const VkPhysicalDevice &physicalDevice, std::vector<VkSurfaceFormatKHR> &surfaceFormatData) {
        unsigned numSurfaceFormats = 0;
        if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface_, &numSurfaceFormats, nullptr) != VK_SUCCESS) {
            throw std::runtime_error("Failed to get number of physical device surface formats.");
        }

        if (numSurfaceFormats != 0) {
            surfaceFormatData = std::vector<VkSurfaceFormatKHR>(numSurfaceFormats);
            if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface_, &numSurfaceFormats, surfaceFormatData.data()) != VK_SUCCESS) {
                throw std::runtime_error("Failed to get physical device surface formats.");
            }
        }
	}

	void Application::GetSupportedPresentationModes(VkPhysicalDevice const &physicalDevice, std::vector<VkPresentModeKHR> &presentationModeData) {
	    unsigned numPresentationModes = 0;
	    if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface_, &numPresentationModes, nullptr) != VK_SUCCESS) {
	        throw std::runtime_error("Failed to get number of physical device presentation modes.");
	    }

	    if (numPresentationModes != 0) {
	        presentationModeData = std::vector<VkPresentModeKHR>(numPresentationModes);
	        if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface_, &numPresentationModes, presentationModeData.data()) != VK_SUCCESS) {
	            throw std::runtime_error("Failed to get physical device presentation modes.");
	        }
	    }
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
		if (vkEnumerateInstanceLayerProperties(&numValidationLayers, nullptr)) {
		    throw std::runtime_error("Failed to get number of instance layer properties.");
		}

		// Get validation layer objects.
		std::vector<VkLayerProperties> availableValidationLayers(numValidationLayers);
		if (vkEnumerateInstanceLayerProperties(&numValidationLayers, availableValidationLayers.data()) != VK_SUCCESS) {
		    throw std::runtime_error("Failed to get instance layer properties.");
		}

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

		QueueFamilyIndexData queueFamilyIndices = FindQueueFamilies(physicalDevice);

		// Check for physical device extensions.
		std::vector<const char*> desiredDeviceExtensions;
		GetDesiredPhysicalDeviceExtensions(desiredDeviceExtensions);

		std::vector<VkExtensionProperties> supportedDeviceExtensions;
		GetSupportedPhysicalDeviceExtensions(physicalDevice, supportedDeviceExtensions);

		bool extensionsSupported = CheckPhysicalDeviceExtensionSupport(physicalDevice, supportedDeviceExtensions, desiredDeviceExtensions);

		// Check for (adequate) support of the swap chain on the physical device.
		bool swapChainSupported = false;
		if (extensionsSupported) {
		    SwapChainSupportData swapChainSupportData = QuerySwapChainSupport(physicalDevice);
		    swapChainSupported = !swapChainSupportData.formats_.empty() && !swapChainSupportData.presentationModes_.empty();
		}

		if (queueFamilyIndices.IsComplete() && extensionsSupported && swapChainSupported) {
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
	QueueFamilyIndexData Application::FindQueueFamilies(const VkPhysicalDevice& physicalDevice) {
		QueueFamilyIndexData queueFamilyIndices;

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

    SwapChainSupportData Application::QuerySwapChainSupport(const VkPhysicalDevice& physicalDevice) {
        SwapChainSupportData data;

        // Basic surface capabilities.
        // Surface is initialized at this point.
        if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface_, &data.surfaceCapabilities_) != VK_SUCCESS) {
            throw std::runtime_error("Failed to get physical device surface capabilities.");
        }

        GetSupportedSurfaceFormats(physicalDevice, data.formats_);
        GetSupportedPresentationModes(physicalDevice, data.presentationModes_);

        return data;
    }

    VkSurfaceFormatKHR Application::ChooseSwapChainSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) {
	    // VkSurfaceFormatKHR contains format and colorSpace members.
	    // Format - color channels and types.
	    // colorSpace - indicates color space

	    for (const VkSurfaceFormatKHR& surfaceFormat : availableFormats) {
	        if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
	            // Found desired SRGB color format + color space.
	            return surfaceFormat;
	        }
	    }

	    // Also possible to rank other available formats based on specification.
	    return availableFormats[0];
    }

    VkPresentModeKHR Application::ChooseSwapChainPresentationMode(const std::vector<VkPresentModeKHR> &availablePresentationModes) {
	    // VK_PRESENT_MODE_IMMEDIATE_KHR - Images submitted are transferred to the screen immediately. Can result in tearing.
	    // VK_PRESENT_MODE_FIFO_KHR - Swap chain is a queue (FIFO) that takes the next image from the front. Images are submitted to the back
	    //                            of the queue. If the queue is full, program has to wait (vsync).
	    // VK_PRESENT_MODE_FIFO_RELAXED_KHR - Same as above. If the queue is empty at the beginning of the refresh, submitted image gets
	    //                                    rendered immediately. Can result in tearing.
        // VK_PRESENT_MODE_MAILBOX_KHR - Same as #2, except when the queue is full, older images are replaced with newer ones (triple buffering).

        for (const VkPresentModeKHR& presentationMode : availablePresentationModes) {
            if (presentationMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                // Attempt to find triple buffering mode.
                return presentationMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR; // Guaranteed to exist.
    }

    VkExtent2D Application::ChooseSwapChainExtent(const VkSurfaceCapabilitiesKHR &surfaceCapabilities) {
	    // Special case: width and height of surfaceCapabilities is (unsigned)-1, which means application window size is not
	    // determined by swap chain size. Should not be modified.
	    if (surfaceCapabilities.currentExtent.width != UINT32_MAX && surfaceCapabilities.currentExtent.height != UINT32_MAX) {
            return surfaceCapabilities.currentExtent;
	    }

	    // Can configure the extent to the size we want (taking into account min/max constraints).
	    int width = -1;
	    int height = -1;

        glfwGetFramebufferSize(window_, &width, &height); // Framebuffer size gets window dimensions in pixels.
                                                          // GLFW uses screen coordinates that do not necessarily translate 1-1.
        VkExtent2D extent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        extent.width = std::clamp(extent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
        extent.height = std::clamp(extent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);

        return extent;
    }

}

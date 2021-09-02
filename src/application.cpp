
// Standard includes.
#include <stdexcept>
#include <vector>
#include <cstring>

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

	void Application::Initialize() {
		InitializeGLFW();

		InitializeVKInstance();
		// Check
	}

	void Application::Update() {
		while (!glfwWindowShouldClose(window_)) {
			glfwPollEvents();
		}
	}

	void Application::Shutdown() {
		vkDestroyInstance(instance_, nullptr); // Optional callback.
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
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Vulkan Tutorial";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "N/A";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_2;

		// Tells the graphics driver which global extensions and validation layers to use.
		// Global (applies to the entire program).
		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		// Get supported extensions.
		std::vector<VkExtensionProperties> supportedExtensions;
		GetSupportedExtensions(supportedExtensions);

		// Get
		std::vector<const char*> desiredExtensions;
		GetDesiredExtensions(desiredExtensions);

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

			createInfo.ppEnabledLayerNames = validationLayerNames.data();
			createInfo.enabledLayerCount = validationLayerNames.size();
		}
		else {
			createInfo.enabledLayerCount = 0; // Don't use any validation layers.
		}

		// Create Vulkan instance.
		if (vkCreateInstance(&createInfo, nullptr, &instance_) != VK_SUCCESS) {
			throw std::runtime_error("failed to create instance!");
		}
	}

	bool Application::CheckInstanceExtensions(const std::vector<VkExtensionProperties>& supportedExtensions, const std::vector<const char*>& desiredExtensions) {
		// Verify extensions exist with supported extensions.
		for (const char* desiredExtension : desiredExtensions) {
			bool found = false;

			for (const VkExtensionProperties& supportedExtension : supportedExtensions) {
				if (strcmp(desiredExtension, supportedExtension.extensionName) == 0) {
					found = true;
					break;
				}
			}

			if (!found) {
				return false;
			}
		}

		return true;
	}

	void Application::GetSupportedExtensions(std::vector<VkExtensionProperties>& extensionData) {
		unsigned extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr); // Get the number of extensions.

		extensionData.resize(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensionData.data()); // Get extension data.
	}

	bool Application::CheckValidationLayers(std::vector<const char*>& validationLayerNames) {
		// Get the number of validation layers
		unsigned numValidationLayers = 0;
		vkEnumerateInstanceLayerProperties(&numValidationLayers, nullptr);

		// Get validation layer objects.
		std::vector<VkLayerProperties> availableValidationLayers(numValidationLayers);
		vkEnumerateInstanceLayerProperties(&numValidationLayers, availableValidationLayers.data());

		// Compare supported validation layers with desired.
		for (const char* desired : validationLayerNames) {
			bool found = false;

			for (const VkLayerProperties& layerProperties : availableValidationLayers) {
				if (strcmp(desired, layerProperties.layerName) == 0) {
					found = true;
					break;
				}
			}

			// Not all desired layers are supported.
			if (!found) {
				return false;
			}
		}

		return true;
	}

	void Application::GetDesiredExtensions(std::vector<const char*>& extensions) {
		unsigned glfwExtensionCount = 0;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		extensions = std::vector<const char*>(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (enableValidationLayers_) {
			// Emplace debug logger extension.
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
	}

}


#ifndef VULKAN_TUTORIAL_APPLICATION_H
#define VULKAN_TUTORIAL_APPLICATION_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>
#include <vector>

#include "physical_device_data.h"

namespace VT {

	class Application {
		public:
			Application(int width, int height);
			~Application();

			void Run();

		private:
			// Functions.
			static VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessageCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData);

			void Initialize();
			void Update();
			void Shutdown();

			void InitializeGLFW();
			void InitializeVKInstance();
			void InitializeDebugMessenger();
			void InitializePhysicalDevice();
			void InitializeLogicalDevice();
			void InitializeVulkanSurface();

			void SetupDebugMessengerUtils(VkDebugUtilsMessengerCreateInfoEXT& messengerInfo);

			void GetSupportedExtensions(std::vector<VkExtensionProperties>& extensionData);
			void GetDesiredInstanceExtensions(std::vector<const char*>& extensions);
			void GetDesiredPhysicalDeviceExtensions(std::vector<const char*>& deviceExtensions);
			void GetSupportedPhysicalDevices(std::vector<VkPhysicalDevice>& deviceData);
			void GetSupportedQueueFamilies(const VkPhysicalDevice& physicalDevice, std::vector<VkQueueFamilyProperties>& queueData);
			void GetSupportedPhysicalDeviceExtensions(const VkPhysicalDevice& physicalDevice, std::vector<VkExtensionProperties>& extensions);

			bool CheckInstanceExtensions(const std::vector<VkExtensionProperties>& supportedExtensions, const std::vector<const char*>& desiredExtensions);
			bool CheckValidationLayers(std::vector<const char*>& validationLayerData);
			bool CheckPhysicalDevices(const std::vector<VkPhysicalDevice>& physicalDevices);
			bool CheckPhysicalDevice(const VkPhysicalDevice& physicalDevice);
			bool CheckPhysicalDeviceExtensionSupport(const VkPhysicalDevice& physicalDevice,  const std::vector<VkExtensionProperties>& supportedDeviceExtensions, const std::vector<const char*>& desiredDeviceExtensions);

			QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice& physicalDevice);

			// Data.
			int width_;
			int height_;

			bool enableValidationLayers_;
			std::vector<const char*> validationLayerNames_;

			GLFWwindow* window_;

			// Vulkan data.
			VkInstance instance_;
			VkSurfaceKHR surface_;
			VkDebugUtilsMessengerEXT messenger_;

			PhysicalDeviceData physicalDeviceData_;

			VkDevice logicalDevice_;
			VkQueue graphicsQueue_;
			VkQueue presentationQueue_;
	};

}

#endif //VULKAN_TUTORIAL_APPLICATION_H

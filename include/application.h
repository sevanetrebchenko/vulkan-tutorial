
#ifndef VULKAN_TUTORIAL_APPLICATION_H
#define VULKAN_TUTORIAL_APPLICATION_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

#include "physical_device_data.h"
#include "swap_chain_support_data.h"

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
			void InitializeSwapChain();
			void InitializeImageViews();
			void InitializeGraphicsPipeline();

			void SetupDebugMessengerUtils(VkDebugUtilsMessengerCreateInfoEXT& messengerInfo);

			void GetSupportedExtensions(std::vector<VkExtensionProperties>& extensionData);
			void GetDesiredInstanceExtensions(std::vector<const char*>& extensions);
			void GetDesiredPhysicalDeviceExtensions(std::vector<const char*>& deviceExtensions);
			void GetSupportedPhysicalDevices(std::vector<VkPhysicalDevice>& deviceData);
			void GetSupportedQueueFamilies(const VkPhysicalDevice& physicalDevice, std::vector<VkQueueFamilyProperties>& queueData);
			void GetSupportedPhysicalDeviceExtensions(const VkPhysicalDevice& physicalDevice, std::vector<VkExtensionProperties>& extensions);
            void GetSupportedSurfaceFormats(const VkPhysicalDevice& physicalDevice, std::vector<VkSurfaceFormatKHR>& surfaceFormatData);
            void GetSupportedPresentationModes(const VkPhysicalDevice& physicalDevice, std::vector<VkPresentModeKHR>& presentationModeData);

			bool CheckInstanceExtensions(const std::vector<VkExtensionProperties>& supportedExtensions, const std::vector<const char*>& desiredExtensions);
			bool CheckValidationLayers(std::vector<const char*>& validationLayerData);
			bool CheckPhysicalDevices(const std::vector<VkPhysicalDevice>& physicalDevices);
			bool CheckPhysicalDevice(const VkPhysicalDevice& physicalDevice);
			bool CheckPhysicalDeviceExtensionSupport(const VkPhysicalDevice& physicalDevice,  const std::vector<VkExtensionProperties>& supportedDeviceExtensions, const std::vector<const char*>& desiredDeviceExtensions);

			QueueFamilyIndexData FindQueueFamilies(const VkPhysicalDevice& physicalDevice);
			SwapChainSupportData QuerySwapChainSupport(const VkPhysicalDevice& physicalDevice);
            std::vector<char> ReadFile(const std::string& filename); // Reads SPIR-V shader files.

			VkSurfaceFormatKHR ChooseSwapChainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
			VkPresentModeKHR ChooseSwapChainPresentationMode(const std::vector<VkPresentModeKHR>& availablePresentationModes);
            VkExtent2D ChooseSwapChainExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);

            VkShaderModule CreateShaderModule(const std::vector<char>& shaderCode);

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

			VkSwapchainKHR swapChain_;
			VkFormat swapChainImageFormat_;
			VkExtent2D swapChainExtent_;
            std::vector<VkImage> swapChainImages_; // Created and cleaned up with swapchain creation/destruction.
	        std::vector<VkImageView> swapChainImageViews_;

	        VkRenderPass renderPass_;
	        VkPipelineLayout pipelineLayout_; // Shader uniform values need to be specified within an object at pipeline creation time.
	        VkPipeline graphicsPipeline_;
	};

}

#endif //VULKAN_TUTORIAL_APPLICATION_H

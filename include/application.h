
#ifndef VULKAN_TUTORIAL_APPLICATION_H
#define VULKAN_TUTORIAL_APPLICATION_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>
#include <vector>

namespace VT {

	class Application {
		public:
			Application(int width, int height);
			~Application();

			void Run();

		private:
			void Initialize();
			void Update();
			void Shutdown();

			void InitializeGLFW();
			void InitializeVKInstance();
			void InitializeDebugMessenger();

			void SetupDebugMessengerUtils(VkDebugUtilsMessengerCreateInfoEXT& messengerInfo);

			void GetSupportedExtensions(std::vector<VkExtensionProperties>& extensionData);
			void GetDesiredExtensions(std::vector<const char*>& extensions);

			bool CheckInstanceExtensions(const std::vector<VkExtensionProperties>& supportedExtensions, const std::vector<const char*>& desiredExtensions);
			bool CheckValidationLayers(std::vector<const char*>& validationLayerData);

			static VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessageCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData);

			int width_;
			int height_;

			bool enableValidationLayers_;

			GLFWwindow* window_;

			// Vulkan.
			VkInstance instance_;
			VkDebugUtilsMessengerEXT messenger_;
	};

}

#endif //VULKAN_TUTORIAL_APPLICATION_H

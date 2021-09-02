
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

			void GetSupportedExtensions(std::vector<VkExtensionProperties>& extensionData);
			void GetRequiredExtensions(std::vector<const char*>& extensions);

			bool CheckInstanceExtensions(const std::vector<VkExtensionProperties>& supportedExtensions, const std::vector<const char*>& desiredExtensions);
			bool CheckValidationLayers(std::vector<const char*>& validationLayerData);

			int width_;
			int height_;

			bool enableValidationLayers_;

			GLFWwindow* window_;
			VkInstance instance_;
	};

}

#endif //VULKAN_TUTORIAL_APPLICATION_H

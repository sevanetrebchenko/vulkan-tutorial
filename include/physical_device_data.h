
#ifndef VULKAN_TUTORIAL_PHYSICAL_DEVICE_DATA_H
#define VULKAN_TUTORIAL_PHYSICAL_DEVICE_DATA_H

#include "queue_family_indices.h"

#include <vulkan/vulkan.h>

namespace VT {

	struct PhysicalDeviceData {
		VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties deviceProperties; // Used for basic device properties, like name, type, and supported Vulkan versions.
		VkPhysicalDeviceFeatures deviceFeatures; // Used for optional features like texture compression, 64-bit floats, and multi vieport rendering.
		std::vector<const char*> deviceExtensions_;
		QueueFamilyIndices queueIndices_;
	};

}

#endif //VULKAN_TUTORIAL_PHYSICAL_DEVICE_DATA_H

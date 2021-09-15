
#ifndef VULKAN_TUTORIAL_UBO_H
#define VULKAN_TUTORIAL_UBO_H

#include <glm/glm.hpp>

namespace VT {

	struct UniformBufferObject {
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
	};

}

#endif //VULKAN_TUTORIAL_UBO_H

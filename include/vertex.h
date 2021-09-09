
#ifndef VULKAN_TUTORIAL_VERTEX_H
#define VULKAN_TUTORIAL_VERTEX_H

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <vector>

namespace VT {

	class Vertex {
		public:
			Vertex(const glm::vec3& position, const glm::vec3& color);
			~Vertex();

			static VkVertexInputBindingDescription GetBindingDescription();
			static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();

		private:
			glm::vec3 position_;
			glm::vec3 color_;
	};

}

#endif //VULKAN_TUTORIAL_VERTEX_H

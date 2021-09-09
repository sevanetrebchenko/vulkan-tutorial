
#include "vertex.h"

namespace VT {

	Vertex::Vertex(const glm::vec3& position, const glm::vec3& color) : position_(position),
																		color_(color)
																		{
	}

	Vertex::~Vertex() {
	}

	VkVertexInputBindingDescription Vertex::GetBindingDescription() {
		VkVertexInputBindingDescription bindingDescription { };

		bindingDescription.binding = 0; // Data is interleaved in one array - only one binding in the shader.
		bindingDescription.stride = sizeof(Vertex);

		// VK_VERTEX_INPUT_RATE_VERTEX - Move to the next data entry after each vertex.
		// VK_VERTEX_INPUT_RATE_INSTANCE - Move to the next data entry after each instance (instanced rendering).
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	std::vector<VkVertexInputAttributeDescription> Vertex::GetAttributeDescriptions() {
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2); // One for position, one for color.

		// Position attribute.
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;

		// float - VK_FORMAT_R32_SFLOAT
		// vec2 - VK_FORMAT_R32G32_SFLOAT
		// vec3 - VK_FORMAT_R32G32B32_SFLOAT
		// vec4 - VK_FORMAT_R32G32B32A32_SFLOAT
		// ivec2 - VK_FORMAT_R32G32_SINT
		// uvec4 - VK_FORMAT_R32G32B32A32_UINT
		// double - VK_FORMAT_R64_SFLOAT
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, position_);

		// Position attribute.
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color_);

		return attributeDescriptions;
	}

}

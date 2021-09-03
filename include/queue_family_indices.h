
#ifndef VULKAN_TUTORIAL_QUEUE_FAMILY_INDICES_H
#define VULKAN_TUTORIAL_QUEUE_FAMILY_INDICES_H

#include <optional>

namespace VT {

	struct QueueFamilyIndices {
		[[nodiscard]] bool IsComplete() const;

		std::optional<unsigned> graphicsFamily_;
	};

}

#endif //VULKAN_TUTORIAL_QUEUE_FAMILY_INDICES_H

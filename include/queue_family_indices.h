
#ifndef VULKAN_TUTORIAL_QUEUE_FAMILY_INDICES_H
#define VULKAN_TUTORIAL_QUEUE_FAMILY_INDICES_H

#include <optional>

namespace VT {

	struct QueueFamilyIndices {
		[[nodiscard]] bool IsComplete() const;

		std::optional<unsigned> graphicsFamily_;
		std::optional<unsigned> presentationFamily_; // Queues for handling presenting data to
													 // the screen could be different from the
													 // queue family that does graphics.
	};

}

#endif //VULKAN_TUTORIAL_QUEUE_FAMILY_INDICES_H

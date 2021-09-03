
#include "queue_family_indices.h"

namespace VT {

	bool QueueFamilyIndices::IsComplete() const {
		return graphicsFamily_.has_value() && presentationFamily_.has_value();
	}

}
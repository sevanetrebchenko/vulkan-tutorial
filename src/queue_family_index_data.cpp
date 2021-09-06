
#include "queue_family_index_data.h"

namespace VT {

	bool QueueFamilyIndexData::IsComplete() const {
		return graphicsFamily_.has_value() && presentationFamily_.has_value();
	}

}
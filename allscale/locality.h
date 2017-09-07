
#pragma once

#include <memory>

#include "allscale/api/core/data.h"

#include "allscale/utils/assert.h"
#include "allscale/utils/printer/join.h"
#include "allscale/utils/serializer.h"

namespace allscale {
namespace simulator {


	// --- a dummy locality manager ---

	const int NUM_LOCATIONS = 5;

	using locality_type = std::size_t;

	inline locality_type getNumLocations() {
		return NUM_LOCATIONS;
	}

	inline locality_type& getLocality() {
		static locality_type locality = 0;
		return locality;
	}

	inline void setLocality(locality_type newLocation) {
		assert_lt(newLocation,NUM_LOCATIONS);
		getLocality() = newLocation;
	}


} // end namespace simulator
} // end namespace allscale

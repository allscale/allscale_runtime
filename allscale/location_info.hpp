#ifndef ALLSCALE_LOCATION_INFO
#define ALLSCALE_LOCATION_INFO
#include <allscale/locality.h>

namespace allscale{
    /**
	 * A class to summarize the distribution of a region of data throughout
	 * the network of nodes.
	 */
	template<typename DataItemType>
	struct location_info {

	    using locality_type = simulator::locality_type;
		using region_type = typename DataItemType::region_type;

		/**
		 * A part of data being located on some node.
		 */
		struct part {

			// the region covered by this location
			region_type region;

			// the location covering it
			locality_type location;

			friend std::ostream& operator<<(std::ostream& out, const part& part) {
				return out << part.region << "@" << part.location;
			}
		};

		// the list of parts distributed throughout the system
		std::vector<part> parts;

	public:

		std::size_t getNumParts() const {
			return parts.size();
		}

		void addPart(const region_type& region, locality_type loc) {
			parts.push_back({ region, loc });
		}

		const std::vector<part>& getParts() const {
			return parts;
		}

		friend std::ostream& operator<<(std::ostream& out, const location_info& info) {
			return out << "LocationInfo {" << allscale::utils::join(",",info.parts) << "}";
		}
	};



} // end namespace allscale

#endif

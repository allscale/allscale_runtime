#ifndef ALLSCALE_LOCATION_INFO
#define ALLSCALE_LOCATION_INFO
#include <hpx/include/serialization.hpp>

namespace allscale{
    /**
	 * A class to summarize the distribution of a region of data throughout
	 * the network of nodes.
	 */
	template<typename DataItemType>
	struct location_info {

		using region_type = typename DataItemType::region_type;

		/**
		 * A part of data being located on some node.
		 */
		struct part {

			// the region covered by this location
			region_type region;

			// the location covering it
            std::size_t rank;

            template<typename Archive>
            void serialize(Archive & ar, unsigned)
            {
                ar & region;
                ar & rank;
            }

			friend std::ostream& operator<<(std::ostream& out, const part& part) {
				return out << part.region << "@" << part.rank;
			}
		};

		// the list of parts distributed throughout the system
		std::vector<part> parts_;

	public:

        template<typename Archive>
        void serialize(Archive & ar, unsigned)
        {
            ar & parts_;
        }

		void add_part(region_type const& region, std::size_t rank) {
			parts_.push_back({ region, rank });
		}

		void add_part(region_type&& region, std::size_t rank) {
			parts_.push_back({ std::move(region), rank });
		}

		const std::vector<part>& parts() const {
			return parts_;
		}

		friend std::ostream& operator<<(std::ostream& out, const location_info& info) {
// 			return out << "LocationInfo {" << allscale::utils::join(",",info.parts) << "}";
			return out;
		}
	};



} // end namespace allscale

#endif

#ifndef ALLSCALE_REQUIREMENT_HPP
#define ALLSCALE_REQUIREMENT_HPP

#include <allscale/data_item.hpp>
#include <hpx/include/serialization.hpp>
#include <allscale/access_privilege.hpp>

namespace allscale
{
	template<typename DataItemDescription>
	struct requirement
	{
		using region_type = typename DataItemDescription::region_type;

		requirement()
		{
		}
		requirement(DataItemDescription descr) :
				region_(descr.r_)
		{
		}

		template<typename Archive>
		void serialize(Archive &ar, unsigned) {
			ar & region_;
			ar & item_;
		}
		data_item<DataItemDescription> item_;
		region_type region_;
		access_privilege access_;

	};
}
#endif


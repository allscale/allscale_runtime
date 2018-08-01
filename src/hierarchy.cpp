#include "allscale/hierarchy.hpp"

namespace allscale {
namespace runtime {

	HierarchyAddress HierarchyAddress::getRootOfNetworkSize(std::size_t numa_size, std::size_t size) {
		// do not support empty networks
		assert_ne(0,size);

		// obtain the result
		return { 0, 0, getLayersOn(0, 0, numa_size, size)-1 };
	}

    std::size_t HierarchyAddress::numaCutOff = 0;

	bool HierarchyAddress::check() const {
// 		// to be valid, the last 'layer' digits in the rank must be 0
// 		for(layer_t i=0; i<layer; i++) {
// 			if ((rank>>i) % 2 != 0) return false;
// 		}
// 		// all fine
		return true;
	}

	layer_t HierarchyAddress::getLayersOn(std::size_t rank, std::size_t numa_node, std::size_t numa_size, std::size_t size) {
		assert_lt(rank,size);

        // Calculate numa height
        std::size_t numa_height = 0;
        if (numa_node == 0)
        {
            std::size_t pos = 1;
            while (pos < numa_size)
            {
                numa_height++;
                pos <<=1;
            }
        }

		// if it is the root, we have to compute the ceil(log_2(size))
		if (rank == 0) {
			// compute the height of the hierarchy
			layer_t height = 0;
			std::size_t pos = 1;
			while(pos < size) {
				height++;
				pos <<=1;
			}
			return height+1 + numa_height;
		}

		// otherwise: compute the number of trailing 0s + 1
		layer_t res = 1;
		while(!(rank % 2)) {
			rank >>= 1;
			res++;
		}
		return res + numa_height;
	}

	std::ostream& operator<<(std::ostream& out, const HierarchyAddress& addr) {
		return out << addr.rank << ":" << addr.numa_node << ":" << addr.layer;
	}


} // end of namespace runtime
} // end of namespace allscale

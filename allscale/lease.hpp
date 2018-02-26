#ifndef ALLSCALE_LEASE
#define ALLSCALE_LEASE

#include <allscale/data_item_requirement.hpp>

namespace allscale{

    template<typename DataItemType>
    struct lease : public data_item_requirement<DataItemType> {
        lease () = default;

        lease(std::size_t rank)
          : rank_(rank)
        {}

        lease(const data_item_requirement<DataItemType>& requirement)
          : data_item_requirement<DataItemType>(requirement)
        {
        }

        std::size_t rank_ = std::size_t(-1);
    };
}

#endif

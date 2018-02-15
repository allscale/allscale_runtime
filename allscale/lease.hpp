#ifndef ALLSCALE_LEASE
#define ALLSCALE_LEASE

#include <allscale/data_item_requirement.hpp>

namespace allscale{

    template<typename DataItemType>
    struct lease : public data_item_requirement<DataItemType> {
        lease(std::size_t rank)
          : data_item_requirement<DataItemType>(),
            rank_(rank)
        {}
        lease ()
          : data_item_requirement<DataItemType>(),
            rank_(-1)
        {}

        lease(const data_item_requirement<DataItemType>& requirement)
          : data_item_requirement<DataItemType>(requirement)
          , rank_(-1)
        {
        }

        std::size_t rank_;
    };
}

#endif

#ifndef ALLSCALE_LEASE
#define ALLSCALE_LEASE

#include <allscale/data_item_requirement.hpp>

namespace allscale{

    template<typename DataItemType>
    struct lease : public data_item_requirement<DataItemType> {
        
        lease () : data_item_requirement<DataItemType>() {}

        lease(const data_item_requirement<DataItemType>& requirement)
            : data_item_requirement<DataItemType>(requirement) 
        {
            std::cout<<"muell"<<std::endl;
        }

    };
}

#endif

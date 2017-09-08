#ifndef ALLSCALE_DATA_ITEM_REQUIREMENT
#define ALLSCALE_DATA_ITEM_REQUIREMENT

#include <allscale/data_item_reference.hpp>
#include <allscale/access_mode.hpp>

namespace allscale{

    template<typename DataItemType>
    struct data_item_requirement {
        using ref_type = data_item_reference<DataItemType>;
        using region_type = typename DataItemType::region_type;


        ref_type ref;
        region_type region;
        access_mode mode;

    };


    template<typename DataItemType>
    data_item_requirement<DataItemType> createDataItemRequirement
    (   const data_item_reference<DataItemType>& ref, 
        const typename DataItemType::region_type& region, 
        const access_mode& mode
    ) 
    {
        //instance a data_item_requirement
        return { ref, region, mode };
    }
 

}
#endif

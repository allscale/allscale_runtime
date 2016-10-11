
#ifndef ALLSCALE_DATA_ITEM_DESCRIPTION_HPP
#define ALLSCALE_DATA_ITEM_DESCRIPTION_HPP



#include <utility>

namespace allscale
{
    template <typename Region,typename Fragment, typename CollectionFacade>
    struct data_item_description
    {
        using region_type= Region;
        using fragment_type = Fragment;
        using collection_facade = CollectionFacade;
        data_item_description()
        {}

    };
}


#endif

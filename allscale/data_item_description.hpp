#ifndef ALLSCALE_DATA_ITEM_DESCRIPTION_HPP
#define ALLSCALE_DATA_ITEM_DESCRIPTION_HPP

#include <utility>
#include <hpx/include/serialization.hpp>

namespace allscale
{
    template <typename Region,typename Fragment, typename CollectionFacade>
    struct data_item_description
    {
        using region_type = Region;
        using fragment_type = Fragment;
        using collection_facade = CollectionFacade;
        data_item_description()
        {}

        data_item_description(Region r, Fragment f, CollectionFacade cf) : r_(r) , f_(f) , cf_(cf) {}

        template <typename Archive>
        void serialize(Archive & ar, unsigned)
        {
            ar & r_;
            ar & f_;
            ar & cf_;
        }

        Region r_;
        Fragment f_;
        CollectionFacade cf_;

    };
}


#endif

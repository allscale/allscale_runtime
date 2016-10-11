#ifndef ALLSCALE_REQUIREMENT_HPP
#define ALLSCALE_REQUIREMENT_HPP

#include <allscale/data_item.hpp>


namespace allscale{

    template <typename DataItemDescription>
    struct requirement
    {
        using region_type = typename DataItemDescription::region_type;
        data_item<DataItemDescription> item_;
        region_type region_;
    };
}
#endif


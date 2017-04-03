#ifndef ALLSCALE_DO_USE_DATA_ITEMS_HPP
#define ALLSCALE_DO_USE_DATA_ITEMS_HPP


namespace allscale
{
    template <typename DataItemDescription>
    struct do_use_data_items
    {
        using data_item_descr = DataItemDescription;
        static constexpr bool activated = true;
    };
}

#endif


#ifndef ALLSCALE_WORK_ITEM_DESCRIPTION_HPP
#define ALLSCALE_WORK_ITEM_DESCRIPTION_HPP

#include <tuple>

namespace allscale
{
    template <
        typename Result,
        typename Name,
        typename SplitVariant,
        typename ProcessVariant,
        typename ...WorkItemVariant
    >
    struct work_item_description
    {
        using result_type = Result;

        using split_variant = SplitVariant;
        using process_variant = ProcessVariant;
        using work_items = std::tuple<WorkItemVariant...>;

        static const char* name()
        {
            return Name::name();
        }
    };
}

#endif


#ifndef ALLSCALE_WORK_ITEM_DESCRIPTION_HPP
#define ALLSCALE_WORK_ITEM_DESCRIPTION_HPP

#include <tuple>


namespace allscale
{
    namespace detail {
        struct can_split_default
        {
            template <typename Closure>
            static bool call(Closure&&)
            {
                return true;
            }
        };

    }
    template <
        typename Result,
        typename Name,
        typename SerVariant,
        typename SplitVariant,
        typename ProcessVariant,
        typename ...WorkItemVariant
    >
    struct work_item_description
    {
        using result_type = Result;
        using ser_variant = SerVariant;
        using split_variant = SplitVariant;
        using process_variant = ProcessVariant;
        using can_split_variant = detail::can_split_default;
        using work_items = std::tuple<WorkItemVariant...>;

        static const char* name()
        {
            return Name::name();
        }
    };

    template <
        typename Result,
        typename Name,
        typename SerVariant,
        typename SplitVariant,
        typename ProcessVariant,
        typename CanSplitVariant,
        typename ...WorkItemVariant
    >
    struct work_item_description<
        Result,
        Name,
        SerVariant,
        SplitVariant,
        ProcessVariant,
        CanSplitVariant,
        WorkItemVariant...>
    {
        using result_type = Result;
        using ser_variant = SerVariant;
        using split_variant = SplitVariant;
        using process_variant = ProcessVariant;
        using can_split_variant = CanSplitVariant;
        using work_items = std::tuple<WorkItemVariant...>;

        static const char* name()
        {
            return Name::name();
        }
    };



    template <
        typename Result,
        typename Name,
        typename SerVariant,
        typename DataItemVariant,
        typename SplitVariant,
        typename ProcessVariant,
        typename CanSplitVariant,
        typename ...WorkItemVariant
    >
    struct work_item_description<
        Result,
        Name,
        SerVariant,
        SplitVariant,
        DataItemVariant,
        ProcessVariant,
        CanSplitVariant,
        WorkItemVariant...>
    {
        using result_type = Result;
        using ser_variant = SerVariant;
        using data_item_variant = DataItemVariant;
        using split_variant = SplitVariant;
        using process_variant = ProcessVariant;
        using can_split_variant = CanSplitVariant;
        using work_items = std::tuple<WorkItemVariant...>;

        static const char* name()
        {
            return Name::name();
        }
    };




}

#endif

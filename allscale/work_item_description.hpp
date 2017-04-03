
#ifndef ALLSCALE_WORK_ITEM_DESCRIPTION_HPP
#define ALLSCALE_WORK_ITEM_DESCRIPTION_HPP

#include <tuple>
#include <allscale/data_item_base.hpp>
#include <allscale/region.hpp>
#include <allscale/fragment.hpp>
#include <allscale/data_item.hpp>
#include <allscale/data_item_description.hpp>

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
        struct data_item_default
        {
        	typedef allscale::region<int> my_region;
        	using my_fragment = allscale::fragment<my_region,std::vector<int>>;
        	typedef allscale::data_item_description<my_region, my_fragment, int> descr;
        	using data_item_descr = descr;
        	static constexpr bool activated = true;

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
        using data_item_variant = detail::data_item_default::data_item_descr;

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
        using data_item_variant = detail::data_item_default::data_item_descr;

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

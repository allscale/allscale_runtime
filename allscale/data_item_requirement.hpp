#ifndef ALLSCALE_DATA_ITEM_REQUIREMENT
#define ALLSCALE_DATA_ITEM_REQUIREMENT

#include <allscale/data_item_reference.hpp>
#include <allscale/access_mode.hpp>
#include <hpx/runtime/serialization/input_archive.hpp>
#include <hpx/runtime/serialization/output_archive.hpp>

namespace allscale{

    template<typename DataItemType>
    struct data_item_requirement {
    public:
        using ref_type = data_item_reference<DataItemType>;
        using region_type = typename DataItemType::region_type;
        using data_item_type = DataItemType;

        ref_type ref;
        region_type region;
        access_mode mode;

        data_item_requirement()
          : mode(access_mode::Invalid)
        {
        }

        data_item_requirement(
                const data_item_reference<DataItemType>& pref,
                const typename DataItemType::region_type& pregion,
                const access_mode& pmode)
          : ref(pref) , region(pregion) , mode(pmode)
        {}

        template <typename Archive>
        void serialize(Archive& ar, unsigned)
        {
           ar & ref;
           ar & region;
           ar & mode;
        }
    };


    template<typename DataItemType>
    data_item_requirement<DataItemType> createDataItemRequirement
    (   const data_item_reference<DataItemType>& ref,
        const typename DataItemType::region_type& region,
        access_mode mode
    )
    {
        //instance a data_item_requirement
        return { ref, region, mode };
    }


}
#endif

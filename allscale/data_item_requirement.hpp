#ifndef ALLSCALE_DATA_ITEM_REQUIREMENT
#define ALLSCALE_DATA_ITEM_REQUIREMENT

#include <allscale/data_item_reference.hpp>
#include <allscale/access_mode.hpp>
//#include <allscale/utils/serializer.h>
//#include <hpx/runtime/serialization/input_archive.hpp>
//#include <hpx/runtime/serialization/output_archive.hpp>
#include <hpx/include/serialization.hpp>
namespace allscale{

    template<typename DataItemType>
    struct data_item_requirement {
    public:
        using ref_type = data_item_reference<DataItemType>;
        using region_type = typename DataItemType::region_type;


        ref_type ref;
        region_type region;
        access_mode mode;

        data_item_requirement(){}
         
        data_item_requirement(const data_item_reference<DataItemType>& pref, 
        const typename DataItemType::region_type& pregion, 
        const access_mode& pmode) : ref(pref) , region(pregion) , mode(pmode){}
/*       void serialize(hpx::serialization::output_archive & ar,unsigned)
        {
            ar & ref;
            ar & mode;
            auto archive = allscale::utils::serialize(region);
            using buffer_type = std::vector<char>;
            buffer_type buffer;
            buffer = archive.getBuffer();
            ar & buffer;
            std::cout<<"ser called"<<std::endl;

        }
        
        void serialize(hpx::serialization::input_archive & ar,unsigned)
        {
            ar & ref;
            ar & mode;
            using buffer_type = std::vector<char>;
            buffer_type buffer;
            ar & buffer;
            allscale::utils::Archive received(buffer);
            region  = allscale::utils::deserialize<region_type>(received);
        }

*/

        template <typename Archive>
        void serialize(Archive& ar, unsigned)
        {
           ar & ref;

           ar & region;
           //ar & mode;
        }
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

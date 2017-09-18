#ifndef ALLSCALE_DATA_ITEM_REFERENCE_HPP
#define ALLSCALE_DATA_ITEM_REFERENCE_HPP

#include <hpx/include/components.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/runtime/serialization/serialize.hpp>
#include <hpx/runtime/serialization/input_archive.hpp>
#include <hpx/runtime/serialization/output_archive.hpp>


using id_type = std::size_t;

///////////////////////////////////////////////////////////////////////////////
struct stub_comp_server : hpx::components::simple_component_base<stub_comp_server>
{
};


///////////////////////////////////////////////////////////////////////////

namespace allscale {
    template<typename DataItemType>
    class data_item_reference  {   
    public:
        data_item_reference() 
        {

            id_ = hpx::new_<stub_comp_server>(hpx::find_here()).get();
        }
        
        void serialize(Archive& ar,const unsigned int){
            ar & id_;
        }
        
        hpx::id_type id_;
        id_type id;
    };
}

#endif

#ifndef ALLSCALE_DATA_ITEM_REFERENCE_HPP
#define ALLSCALE_DATA_ITEM_REFERENCE_HPP

#include <hpx/include/components.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/serialization.hpp>

//using id_type = std::size_t;


/*
///////////////////////////////////////////////////////////////////////////////
struct stub_comp_server : hpx::components::simple_component_base<stub_comp_server>
{
};
*/
struct stub_comp_server : public hpx::components::locking_hook<
            hpx::components::component_base<stub_comp_server> >
{
};




/*


        typedef hpx::components::client_base<
            template_accumulator<T>, server::template_accumulator<T>
        > base_type;

*/
///////////////////////////////////////////////////////////////////////////

namespace allscale {
    template<typename DataItemType>
    class data_item_reference  {   
    public:
        data_item_reference() 
        {
            id_ = hpx::new_<stub_comp_server>(hpx::find_here()).get();
        }
        
        data_item_reference(const data_item_reference& ref)
        {
            id_ = ref.id_;
        }
        template <typename Archive>
        void serialize(Archive & ar, unsigned)
        {
        
        }
    
        hpx::id_type id_;
 //       id_type id;
    };
}

#endif

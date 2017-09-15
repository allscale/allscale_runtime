#ifndef ALLSCALE_DATA_ITEM_REFERENCE_HPP
#define ALLSCALE_DATA_ITEM_REFERENCE_HPP


using id_type = std::size_t;

// DATA ITEM REFERENCE SERVER CLASS and CLIENT CLASS  AND MACRO DEFINITIONS
namespace allscale{
namespace server {

template<typename DataItemType>
class data_item_reference: public hpx::components::locking_hook<
        hpx::components::component_base<data_item_reference<DataItemType> > > {

public:
    using data_item_type = DataItemType;

    data_item_reference() :
            value_(0) {
    }

    ///////////////////////////////////////////////////////////////////////
    // Exposed functionality of this component.

    /// Reset the components value to 0.
    void print() {
        std::cout << "Locality: "
                << hpx::get_locality_id() << " Id_type: " << this->get_id() << std::endl;
    }

    ///////////////////////////////////////////////////////////////////////
    // Each of the exposed functions needs to be encapsulated into an
    // action type, generating all required boilerplate code for threads,
    // serialization, etc.

    void set_value(int val) {
        value_ = val;
    }

    ///////////////////////////////////////////////////////////////////////
    // Each of the exposed functions needs to be encapsulated into an
    // action type, generating all required boilerplate code for threads,
    // serialization, etc.
    HPX_DEFINE_COMPONENT_ACTION(data_item_reference, print);HPX_DEFINE_COMPONENT_ACTION(data_item_reference, set_value);

    id_type id;
    hpx::id_type id_;

    int value_;
};

}}

#define REGISTER_DATAITEMREFERENCE_DECLARATION(type)                       \
    HPX_REGISTER_ACTION_DECLARATION(                                          \
        allscale::server::data_item_reference<type>::print_action,           \
        BOOST_PP_CAT(__data_item_reference_print_action_, type));            \
    HPX_REGISTER_ACTION_DECLARATION(                                          \
        allscale::server::data_item_reference<type>::set_value_action,           \
        BOOST_PP_CAT(__data_item_reference_set_value_action_, type));     \

/**/

#define REGISTER_DATAITEMREFERENCE(type)                                   \
    HPX_REGISTER_ACTION(                                                      \
        allscale::server::data_item_reference<type>::print_action,           \
        BOOST_PP_CAT(__data_item_reference_print_action_, type));            \
    HPX_REGISTER_ACTION(                                                      \
        allscale::server::data_item_reference<type>::set_value_action,           \
        BOOST_PP_CAT(__data_item_reference_set_value_action_, type));            \
    typedef ::hpx::components::component<                                     \
        allscale::server::data_item_reference<type>                         \
    > BOOST_PP_CAT(__data_item_reference_, type);                            \
    HPX_REGISTER_COMPONENT(BOOST_PP_CAT(__data_item_reference_, type))       \
/**/

namespace allscale {

///////////////////////////////////////////////////////////////////////////
/// Client for the data_item_reference component type 
template<typename DataItemType>
class data_item_reference: public hpx::components::client_base<
        data_item_reference<DataItemType>, server::data_item_reference<DataItemType> > {
    typedef hpx::components::client_base<data_item_reference<DataItemType>,
            server::data_item_reference<DataItemType> > base_type;

    typedef typename server::data_item_reference<DataItemType>::data_item_type data_item_type;

public:
    data_item_reference() {
    }

    data_item_reference(hpx::future<hpx::id_type> && gid) :
            base_type(std::move(gid)) {
    }

    /// print shit with fire and forget
    void print(hpx::launch::apply_policy) {
        HPX_ASSERT(this->get_id());

        typedef typename server::data_item_reference<DataItemType>::print_action action_type;
        hpx::apply < action_type > (this->get_id());
    }

    void print() {
        HPX_ASSERT(this->get_id());

        typedef typename server::data_item_reference<DataItemType>::print_action action_type;
        action_type()(this->get_id());
    }

    /// set_value
    void set_value(int val) {
        HPX_ASSERT(this->get_id());

        typedef typename server::data_item_reference<DataItemType>::set_value_action action_type;
        action_type()(this->get_id(), val);
    }
    id_type id;

};

}

#endif

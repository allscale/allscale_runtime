#ifndef ALLSCALE_DATA_ITEM_MANAGER_SERVER_HPP
#define ALLSCALE_DATA_ITEM_MANAGER_SERVER_HPP

#include <allscale/data_item.hpp>
#include <allscale/data_item_description.hpp>
#include <allscale/components/data_item_manager_server.hpp>
#include <allscale/data_item_base.hpp>

#include <allscale/requirement.hpp>

#include <hpx/include/future.hpp>
#include <hpx/include/lcos.hpp>
#include <hpx/include/serialization.hpp>

//#include <boost/shared_ptr.hpp>
//#include <boost/move/unique_ptr.hpp>
//#include <boost/pointer_cast.hpp>
#include <memory>
#include <unordered_map>





namespace allscale
{



    struct data_item_manager_server
        : public hpx::components::client_base<data_item_manager_server, components::data_item_manager_server >
    {
        public:
        using base_type = hpx::components::client_base<data_item_manager_server, components::data_item_manager_server >;

        using get_left_neighbor_action = typename allscale::components::data_item_manager_server::get_left_neighbor_action;
      	using get_right_neighbor_action = typename allscale::components::data_item_manager_server::get_right_neighbor_action;
        using set_left_neighbor_action = typename allscale::components::data_item_manager_server::set_left_neighbor_action;
		using set_right_neighbor_action = typename allscale::components::data_item_manager_server::set_right_neighbor_action;




        data_item_manager_server ()
        {
        }

        data_item_manager_server(hpx::id_type loc)
            : base_type(hpx::new_<components::data_item_manager_server > (loc))
        {
            servers_home_locality_ = loc;
        }



        // create a data_item on the locality where this data_item_manager_server lives on ( this->get_id())
        // uses future continuation, but does still block right now due to the f2.get()
        template <typename DataItemDescription>
       // std::shared_ptr<allscale::data_item<DataItemDescription>>
        bool
        create_data_item (DataItemDescription arg)
        {
            HPX_ASSERT(this->valid());
            using data_item_type = typename  allscale::data_item<DataItemDescription>;
            using action_type = typename components::data_item_manager_server::create_data_item_async_action<DataItemDescription>;
            auto ret_val = hpx::async<action_type>(this->get_id(),arg);

//             hpx::future<data_item_type> f2 = ret_val.then(
//                 [this](hpx::future<data_item_type> f) -> data_item_type
//                 {
//                        return f.get();
//                 });

            return ret_val.is_ready();
        }




        // create a data_item on the locality where this data_item_manager_server lives on ( this->get_id())
            // uses future continuation, but does still block right now due to the f2.get()
            template <typename DataItemDescription>
           // std::shared_ptr<allscale::data_item<DataItemDescription>>
            bool
            create_data_item ()
            {
                HPX_ASSERT(this->valid());
                using data_item_type = typename  allscale::data_item<DataItemDescription>;
                using action_type = typename components::data_item_manager_server::create_empty_data_item_async_action<DataItemDescription>;
                auto ret_val = hpx::async<action_type>(this->get_id());

    //             hpx::future<data_item_type> f2 = ret_val.then(
    //                 [this](hpx::future<data_item_type> f) -> data_item_type
    //                 {
    //                        return f.get();
    //                 });

                return ret_val.is_ready();
            }






        template<typename DataItemDescription>
        std::vector<hpx::future<std::vector<std::pair<typename DataItemDescription::region_type, hpx::naming::id_type> > > >
        locate ( allscale::requirement<DataItemDescription>& requirement)
        {
            HPX_ASSERT(this->valid());

           // std::cout<<"locate called" << std::endl;
            using action_type = typename components::data_item_manager_server::locate_async_action<DataItemDescription>;
            using fut_type = decltype(hpx::async<action_type>(this->get_id(),requirement));
            std::vector<fut_type> results;
/*
            results.push_back(hpx::async<action_type>(this->get_id(),requirement));
            results.push_back(hpx::async<action_type>(left_,requirement));
            results.push_back(hpx::async<action_type>(right_,requirement));
*/


//            for(auto& el : servers_){
//                results.push_back(hpx::async<action_type>(el,requirement));
//            }
            //auto ret_val = hpx::async<action_type>(this->get_id(),requirement);
            return results;
        }

//        template<typename DataItemDescription>
//        hpx::future<typename DataItemDescription::collection_facade>
//        acquire( typename DataItemDescription::region_type const& region)
//        {
//        	using action_type = typename components::data_item_manager_server::acquire_async_action<DataItemDescription>;
//        	return hpx::async<action_type>(this->get_id(),region);
//        }

    	template<typename Archive>
    	void serialize(Archive & ar, unsigned) {
    		ar & servers_;
    		ar & left_;
    		ar & right_;
    		ar & servers_home_locality_;
    	}



        //TODO: function returning mask to data_item usw
        // get_access(data_item_id, region r)
        //
        // data item selber liegt in der closure
        template <typename DataItemDescription>
        void destroy (data_item<DataItemDescription> item)
        {
            //destroy it
        }
        
        std::vector<hpx::naming::id_type> servers_;
        hpx::naming::id_type left_;
        hpx::naming::id_type  right_;
        hpx::naming::id_type servers_home_locality_;


    };

}




/*
#define ALLSCALE_REGISTER_DATA_ITEM_MANAGER_SERVER_TYPE_(T, NAME)            \
    using NAME =                                                                \
        ::hpx::components::managed_component<                                   \
            ::allscale::components::data_item_manager_server< T>            \
        >;                                                                      \
    HPX_REGISTER_COMPONENT(NAME)                                                \

#define ALLSCALE_REGISTER_DATA_ITEM_MANAGER_SERVER_TYPE(T)                   \
    ALLSCALE_REGISTER_DATA_ITEM_MANAGER_SERVER_TYPE_(                                           \
        T,                                                                      \
        BOOST_PP_CAT(BOOST_PP_CAT(data_item_manager_server, T), _component_type)                \
    )                                                                           \
*/


/*
#define ALLSCALE_REGISTER_DATA_ITEM_MANAGER_SERVER_DECLARATION(type)                       \
    HPX_REGISTER_ACTION_DECLARATION(                                          \
        allscale::components::data_item_manager_server<type>::print_greating_action,           \
        BOOST_PP_CAT(__data_item_manager_server_print_greating_action_, type));            \
    HPX_REGISTER_ACTION_DECLARATION(                                          \
        allscale::components::data_item_manager_server<type>::create_remote_action,           \
        BOOST_PP_CAT(__data_item_manager_server_create_remote_action_, type));            \
*/
/**/

#define ALLSCALE_REGISTER_DATA_ITEM_MANAGER_SERVER_COMPONENT()     \
    using NAME =                                                    \
        ::hpx::components::managed_component<                       \
            ::allscale::components::data_item_manager_server   \
        >;                                                          \
    HPX_REGISTER_COMPONENT(NAME)    \
/**/




/*
#define ALLSCALE_REGISTER_DATA_ITEM_MANAGER_SERVER_TYPE_(T,NAME)     \
    HPX_REGISTER_ACTION(                                                \
            allscale::components::data_item_manager_server<T>::print_greating_action, \
            BOOST_PP_CAT(__data_item_manager_server_print_greating_action, type)); \
                                                                    \
                                                                    \
    using NAME =                                                    \
        ::hpx::components::managed_component<                       \
            ::allscale::components::data_item_manager_server< T >    \
        >;                                                          \
    HPX_REGISTER_COMPONENT(NAME)                                    \


#define ALLSCALE_REGISTER_DATA_ITEM_MANAGER_SERVER_TYPE(T)          \
    ALLSCALE_REGISTER_DATA_ITEM_MANAGER_SERVER_TYPE_(               \
        T,                                                          \
        BOOST_PP_CAT(BOOST_PP_CAT(data_item_manager_server, T), _component_type)     \
        )

        */
#endif

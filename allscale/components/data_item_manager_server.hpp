#ifndef ALLSCALE_COMPONENTS_DATA_ITEM_MANAGER_SERVER_HPP
#define ALLSCALE_COMPONENTS_DATA_ITEM_MANAGER_SERVER_HPP

#include <hpx/include/components.hpp>
#include <hpx/hpx.hpp>
#include <memory>
#include <allscale/data_item_base.hpp>
#include <allscale/requirement.hpp>
#include <vector>
namespace allscale {
namespace components {
struct data_item_manager_server: hpx::components::managed_component_base<
		data_item_manager_server> {
	typedef hpx::components::managed_component_base<data_item_manager_server> server_type;

	template<typename T>
	hpx::future<std::shared_ptr<allscale::data_item<T>>>create_data_item_async( T arg)
	{
		using data_item_type = allscale::data_item<T>;
		hpx::lcos::local::promise<std::shared_ptr<data_item_type>> promise_;

//            std::shared_ptr<data_item_base> ptr (new data_item_type(hpx::find_here(), arg));
		std::shared_ptr<data_item_base> ptr = std::make_shared<data_item_type> (data_item_type(hpx::find_here(), arg));

//            data_item_type k1 = *(std::static_pointer_cast<data_item_type>(ptr));
//            data_item_type k2 = *(std::static_pointer_cast<data_item_type>(ptr2));
//
//            std::cout<< " ptrdd : " << *(k1.fragment_.ptr_) << std::endl;
		//std::cout<< " ptr2 : " << *(k2.fragment_.ptr_) << std::endl;

		//auto ptr = std::make_shared<data_item_base>(data_item_type(hpx::find_here(), arg));
		local_data_items.push_back(ptr);
		std::shared_ptr<data_item_type> k = std::static_pointer_cast<data_item_type>(ptr);
		//data_item_type k2 = *(std::static_pointer_cast<data_item_type>(ptr));

		promise_.set_value(k);
		return promise_.get_future();
	}

	template <typename T>
	struct create_data_item_async_action
	: hpx::actions::make_action<
	hpx::future<std::shared_ptr<allscale::data_item<T>>>(data_item_manager_server::*)(T),
	&data_item_manager_server::template create_data_item_async<T>,
	create_data_item_async_action<T>
	>
	{};

	/*
	 template <typename T>
	 hpx::future<T>
	 locate_async( allscale::requirement<T> requirement){
	 hpx::lcos::local::promise<T> promise_;
	 //           auto target_region = requirement.region_;
	 //
	 //           for( std::shared_ptr<data_item_base> base_item : local_data_items )
	 //           {
	 //               std::cout<< "djkkawjdkdaw" << std::endl;
	 //           }
	 future_type res;
	 return promise_.get_future();
	 }


	 template <typename T>
	 struct locate_async_action
	 : hpx::actions::make_action<
	 hpx::future<T> (data_item_manager_server::*)(allscale::requirement<T>),
	 &data_item_manager_server::template locate_async<T>,
	 locate_async_action<T>
	 >
	 {};


	 */
	template <typename DataItemDescription>
	hpx::future<std::vector<std::pair<typename DataItemDescription::region_type, hpx::naming::id_type> > >
	locate_async( allscale::requirement<DataItemDescription> requirement) {
        
		using future_type = std::vector<std::pair<typename DataItemDescription::region_type, hpx::naming::id_type> >;
		using pair_type = std::pair<typename DataItemDescription::region_type, hpx::naming::id_type>;

		hpx::lcos::local::promise<future_type> promise_;
		using data_item_type = allscale::data_item<DataItemDescription>;
		auto target_region = requirement.region_;
		future_type tmp2;
		pair_type *target_pair;

		for( std::shared_ptr<data_item_base> base_item : local_data_items )
		{
			data_item_type tmp = *(std::static_pointer_cast<data_item_type>(base_item));
			if (tmp.region_.has_intersection_with(target_region))
			{
				//        std::cout<< "Found the data item on locality: " << tmp.parent_loc  << std::endl;
				//target_pair = new pair_type(tmp.region_,tmp.parent_loc);
				target_pair = new pair_type(tmp.region_,this->get_id());

				tmp2.push_back(*(target_pair));
			}
		}
		promise_.set_value(tmp2);
		return promise_.get_future();
	}

	template <typename DataItemDescription>
	struct locate_async_action
	: hpx::actions::make_action<
	hpx::future<std::vector<std::pair<typename DataItemDescription::region_type, hpx::naming::id_type> > > (data_item_manager_server::*)(allscale::requirement<DataItemDescription>),
	&data_item_manager_server::template locate_async<DataItemDescription>,
	locate_async_action<DataItemDescription>
	>
	{};

	template<typename DataItemDescription>
	hpx::future<typename DataItemDescription::collection_facade> acquire_async ( typename DataItemDescription::region_type const& region)
	{
//		std::cout<<"acquire is called for region: "<< region.to_string() << std::endl;
		using future_type = typename DataItemDescription::collection_facade;
		future_type tmp2;
		hpx::lcos::local::promise<future_type> promise_;

		using data_item_type = allscale::data_item<DataItemDescription>;
		for(std::shared_ptr<data_item_base> base_item : local_data_items) {
			data_item_type tmp = *(std::static_pointer_cast<data_item_type>(base_item));
			if(region == tmp.region_){
//				std::cout<<"found this region exactly"<<std::endl;
				tmp2 = tmp.fragment_.ptr_;
			}

		}

		promise_.set_value(tmp2);
		return promise_.get_future();

	}

	template <typename DataItemDescription>
	struct acquire_async_action
	: hpx::actions::make_action<
	hpx::future<typename DataItemDescription::collection_facade>  (data_item_manager_server::*)(typename DataItemDescription::region_type const&),
	&data_item_manager_server::template acquire_async<DataItemDescription>,
	acquire_async_action<DataItemDescription>
	>
	{};

	std::vector<std::shared_ptr<data_item_base> > local_data_items;
};
}
}

#endif

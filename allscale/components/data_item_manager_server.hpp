#ifndef ALLSCALE_COMPONENTS_DATA_ITEM_MANAGER_SERVER_HPP
#define ALLSCALE_COMPONENTS_DATA_ITEM_MANAGER_SERVER_HPP

#include <hpx/include/components.hpp>
#include <hpx/hpx.hpp>
#include <memory>
#include <allscale/data_item_base.hpp>
#include <allscale/data_item.hpp>
#include <allscale/components/data_item.hpp>
#include <allscale/requirement.hpp>
#include <allscale/fragment.hpp>
#include <vector>
#include <unordered_map>
#include <allscale/fragment.hpp>

namespace allscale {
namespace components {

struct hpx_id_type_hash {
	std::uint64_t operator()(hpx::naming::id_type const& id) const {
		return id.get_lsb();
	}
};

struct data_item_manager_server: hpx::components::managed_component_base<
		data_item_manager_server> {
	typedef hpx::components::managed_component_base<data_item_manager_server> server_type;
	using fragment_base = allscale::fragment_base;
	using map_type = std::unordered_map<hpx::id_type,std::vector<std::shared_ptr<allscale::fragment_base>>,hpx_id_type_hash>;

	template<typename T>
//	hpx::future<std::shared_ptr<allscale::data_item<T>>>create_data_item_async( T arg)
	hpx::future<bool> create_data_item_async(T arg)

	{
		using data_item_type = allscale::data_item<T>;
		//hpx::lcos::local::promise<std::shared_ptr<data_item_type>> promise_;
		hpx::lcos::local::promise<bool> promise_;

		std::shared_ptr<data_item_base> ptr = std::make_shared<data_item_type>(
				data_item_type(hpx::find_here(), arg));
		local_data_items.push_back(ptr);
		std::shared_ptr<data_item_type> k = std::static_pointer_cast<
				data_item_type>(ptr);
		auto id = (*k).get_id();
		std::pair<hpx::id_type,
				std::vector<std::shared_ptr<allscale::fragment_base>>>p(id,std::vector<std::shared_ptr<allscale::fragment_base>>());

		promise_.set_value(true);
		return promise_.get_future();
	}

//	template <typename T>
//	struct create_data_item_async_action
//	: hpx::actions::make_action<
//	hpx::future<std::shared_ptr<allscale::data_item<T>>>(data_item_manager_server::*)(T),
//	&data_item_manager_server::template create_data_item_async<T>,
//	create_data_item_async_action<T>
//	>
//	{};

	template<typename T>
	struct create_data_item_async_action: hpx::actions::make_action<
			hpx::future<bool> (data_item_manager_server::*)(T),
			&data_item_manager_server::template create_data_item_async<T>,
			create_data_item_async_action<T> > {
	};

	template<typename T>
	hpx::future<hpx::id_type> create_empty_data_item_async()

	{
		using data_item_type = allscale::data_item<T>;
		hpx::lcos::local::promise<hpx::id_type> promise_;
		std::shared_ptr<data_item_base> ptr = std::make_shared<data_item_type>(
				data_item_type());

		local_data_items.push_back(ptr);
		std::shared_ptr<data_item_type> k = std::static_pointer_cast<
				data_item_type>(ptr);
		auto id = (*k).get_id();

		std::pair<hpx::id_type,
				std::vector<std::shared_ptr<allscale::fragment_base>>>p(id,std::vector<std::shared_ptr<allscale::fragment_base>>());
		difm.insert(p);

		promise_.set_value(id);
		return promise_.get_future();
	}

	template<typename T>
	struct create_empty_data_item_async_action: hpx::actions::make_action<
			hpx::future<hpx::id_type> (data_item_manager_server::*)(),
			&data_item_manager_server::template create_empty_data_item_async<T>,
			create_empty_data_item_async_action<T> > {
	};

	template<typename DataItemDescription>
	hpx::future<bool> create_fragment_async(hpx::id_type data_item_id,
			typename DataItemDescription::region_type region)

	{
		using fragment_type = typename DataItemDescription::fragment_type;

		map_type::const_iterator got = difm.find(data_item_id);

		if (got == difm.end()) {
			std::cout << data_item_id << " was not found on loc id: "
					<< hpx::get_locality_id() << std::endl;
		}
		else {
			std::shared_ptr<allscale::fragment_base> ptr = std::make_shared<fragment_type> (fragment_type(region,std::vector<int>(10,337)));
            difm[data_item_id].push_back(ptr);
            //std::shared_ptr<allscale::fragment_base> ptr = std::make_shared<typename DataItemDescription::fragment_type>(typename DataItemDescription::fragment_type );
			 // std::cout << data_item_id << " is " << difm[data_item_id].size()
			 // << " loc id: " << hpx::get_locality_id() << std::endl;
		    for( auto & el : difm[data_item_id] ){
                fragment_type frag = *(std::static_pointer_cast<fragment_type>(el));
                std::cout<<"region is : " << frag.region_.to_string() << std::endl;
            }
        }
		hpx::lcos::local::promise<bool> promise_;

		promise_.set_value(true);
		return promise_.get_future();
	}

	template<typename DataItemDescription>
	struct create_fragment_async_action: hpx::actions::make_action<
			hpx::future<bool> (data_item_manager_server::*)(hpx::id_type,
					typename DataItemDescription::region_type),
			&data_item_manager_server::template create_fragment_async<
					DataItemDescription>,
			create_fragment_async_action<DataItemDescription> > {
	};

   template<typename DataItemDescription>
	hpx::future<
			std::vector<
					std::pair<typename DataItemDescription::region_type,
							hpx::naming::id_type> > > locate_async(
			allscale::requirement<DataItemDescription> requirement) {

		using future_type = std::vector<std::pair<typename DataItemDescription::region_type, hpx::naming::id_type> >;
		using pair_type = std::pair<typename DataItemDescription::region_type, hpx::naming::id_type>;

		hpx::lcos::local::promise<future_type> promise_;
		using data_item_type = allscale::data_item<DataItemDescription>;
		auto target_region = requirement.region_;
		future_type tmp2;
		pair_type *target_pair;
		for (std::shared_ptr<data_item_base> base_item : local_data_items) {
			data_item_type tmp = *(std::static_pointer_cast<data_item_type>(
					base_item));
			if (tmp.region_.has_intersection_with(target_region)) {
				//        std::cout<< "Found the data item on locality: " << tmp.parent_loc  << std::endl;
				//target_pair = new pair_type(tmp.region_,tmp.parent_loc);
				target_pair = new pair_type(tmp.region_, this->get_id());

				tmp2.push_back(*(target_pair));
			}
		}
		promise_.set_value(tmp2);
		return promise_.get_future();
	}

	template<typename DataItemDescription>
	struct locate_async_action: hpx::actions::make_action<
			hpx::future<
					std::vector<
							std::pair<typename DataItemDescription::region_type,
									hpx::naming::id_type> > > (data_item_manager_server::*)(
					allscale::requirement<DataItemDescription>),
			&data_item_manager_server::template locate_async<DataItemDescription>,
			locate_async_action<DataItemDescription> > {
	};

	template<typename DataItemDescription>
	hpx::future<typename DataItemDescription::collection_facade> acquire_async(
			typename DataItemDescription::region_type const& region) {
//		std::cout<<"acquire is called for region: "<< region.to_string() << std::endl;
		using future_type = typename DataItemDescription::collection_facade;
		future_type tmp2;
		hpx::lcos::local::promise<future_type> promise_;

		using data_item_type = allscale::data_item<DataItemDescription>;
		for (std::shared_ptr<data_item_base> base_item : local_data_items) {
			data_item_type tmp = *(std::static_pointer_cast<data_item_type>(
					base_item));
			if (region == tmp.region_) {
				std::cout<<
//				std::cout<<"found this region exactly"<<std::endl;
				tmp2 = tmp.fragment_.ptr_;
			}

		}

		promise_.set_value(tmp2);
		return promise_.get_future();

	}

	template<typename DataItemDescription>
	struct acquire_async_action: hpx::actions::make_action<
			hpx::future<typename DataItemDescription::collection_facade> (data_item_manager_server::*)(
					typename DataItemDescription::region_type const&),
			&data_item_manager_server::template acquire_async<
					DataItemDescription>,
			acquire_async_action<DataItemDescription> > {
	};

	template<typename DataItemDescription>
	hpx::future<typename DataItemDescription::fragment_type> acquire_fragment_async(
			typename DataItemDescription::region_type const& region) {
		//		std::cout<<"acquire is called for region: "<< region.to_string() << std::endl;
		using future_type = typename DataItemDescription::fragment_type;
		future_type tmp2;
		hpx::lcos::local::promise<future_type> promise_;

		using data_item_type = allscale::data_item<DataItemDescription>;
		for (std::shared_ptr<data_item_base> base_item : local_data_items) {
			data_item_type tmp = *(std::static_pointer_cast<data_item_type>(
					base_item));
			if (region == tmp.region_) {
				//				std::cout<<"found this region exactly"<<std::endl;
				//hand out a copy of fragment
				tmp2 = tmp.fragment_;
			}

		}

		promise_.set_value(tmp2);
		return promise_.get_future();

	}

	template<typename DataItemDescription>
	struct acquire_fragment_async_action: hpx::actions::make_action<
			hpx::future<typename DataItemDescription::fragment_type> (data_item_manager_server::*)(
					typename DataItemDescription::region_type const&),
			&data_item_manager_server::template acquire_fragment_async<
					DataItemDescription>,
			acquire_fragment_async_action<DataItemDescription> > {
	};

	hpx::id_type get_left_neighbor() {
		return left_;
	}
	HPX_DEFINE_COMPONENT_DIRECT_ACTION(data_item_manager_server, get_left_neighbor);

	hpx::id_type get_right_neighbor() {
		return right_;
	}
	HPX_DEFINE_COMPONENT_DIRECT_ACTION(data_item_manager_server, get_right_neighbor);

	void set_left_neighbor(hpx::naming::id_type l) {
		left_ = l;
	}
	HPX_DEFINE_COMPONENT_DIRECT_ACTION(data_item_manager_server, set_left_neighbor);

	void set_right_neighbor(hpx::naming::id_type r) {
		right_ = r;
	}
	HPX_DEFINE_COMPONENT_DIRECT_ACTION(data_item_manager_server, set_right_neighbor);

	hpx::naming::id_type left_;
	hpx::naming::id_type right_;

	//map of data_items | vector<fragment>
	//std::unordered_map<hpx::naming::id_type,std::vector<std::shared_ptr<fragment_base>>> data_item_fragments_map;
	std::vector<std::shared_ptr<data_item_base> > local_data_items;
	std::vector<std::shared_ptr<allscale::fragment_base> > local_fragments;

	std::unordered_map<hpx::id_type,
			std::vector<std::shared_ptr<allscale::fragment_base>>,
			hpx_id_type_hash> difm;

};
}
}

#endif

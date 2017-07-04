#include <allscale/no_split.hpp>
#include <allscale/no_serialization.hpp>
#include <allscale/do_serialization.hpp>
#include <allscale/do_use_data_items.hpp>
#include <allscale/treeture.hpp>
#include <allscale/spawn.hpp>
#include <allscale/scheduler.hpp>
#include <allscale/work_item_description.hpp>
#include <allscale/requirement.hpp>
#include <allscale/runtime.hpp>
#include <allscale/no_data_items.hpp>
#include <unistd.h>

#include <allscale/data_item.hpp>
#include <allscale/treeture.hpp>
#include <allscale/data_item_description.hpp>
#include <allscale/region.hpp>
#include <allscale/fragment.hpp>
#include <allscale/data_item_manager_server.hpp>
#include <allscale/components/data_item_manager_server.hpp>

namespace allscale {
template<typename T>
struct grid_region_1d: public allscale::region<T> {
public:
	grid_region_1d() :
			begin_(0), end_(0) {
	}

	grid_region_1d(std::size_t b, std::size_t e) :
			begin_(b), end_(e) {
	}
	bool operator==(grid_region_1d const & rh) const {
		return (begin_ == rh.begin_ && end_ == rh.end_);
	}
	bool has_intersection_with(grid_region_1d const & rh) {
		return !((rh.end_ < begin_) || (rh.begin_ > end_));
	}

	template<typename Archive>
	void serialize(Archive & ar, unsigned) {
		ar & begin_;
		ar & end_;

	}

	std::string to_string() const {
		std::string text = "[";
		text += std::to_string(begin_);
		text += ":";
		text += std::to_string(end_);
		text += "]";
		return text;
	}

	std::size_t begin_;
	std::size_t end_;
};
}

using grid_region_type = allscale::grid_region_1d<int>;
using grid_fragment = allscale::fragment<grid_region_type,std::vector<int>>;
using grid_data_item_descr = allscale::data_item_description<grid_region_type,grid_fragment,std::shared_ptr<std::vector<int>>>;
using grid_test_item = allscale::data_item<grid_data_item_descr>;
using grid_test_requirement = allscale::requirement<grid_data_item_descr>;
using value_type = std::vector<int>;

ALLSCALE_REGISTER_DATA_ITEM_TYPE(grid_data_item_descr);

typedef allscale::region<int> my_region;
using my_fragment = allscale::fragment<my_region,std::vector<int>>;
typedef allscale::data_item_description<my_region, my_fragment, int> descr;
typedef allscale::data_item<descr> test_data_item;
ALLSCALE_REGISTER_DATA_ITEM_TYPE(descr);

typedef allscale::data_item_manager_server my_data_item_manager_server;
typedef std::pair<hpx::naming::id_type, my_data_item_manager_server> loc_server_pair;

ALLSCALE_REGISTER_DATA_ITEM_MANAGER_SERVER_COMPONENT()
;

// a handle for loop iterations
class pfor_loop_handle {

	allscale::treeture<void> treeture;

public:

	pfor_loop_handle() :
			treeture(allscale::make_ready_treeture()) {
	}

	pfor_loop_handle(allscale::treeture<void>&& treeture) :
			treeture(std::move(treeture)) {
	}

	pfor_loop_handle(const pfor_loop_handle&) = delete;
	pfor_loop_handle(pfor_loop_handle&&) = default;

	~pfor_loop_handle() {
		if (treeture.valid())
			treeture.wait();
	}

	pfor_loop_handle& operator=(const pfor_loop_handle&) = delete;
	pfor_loop_handle& operator=(pfor_loop_handle&& other) = default;

	const allscale::treeture<void>& get_treeture() const {
		return treeture;
	}
};

template<typename Body>
struct pfor_can_split {
	template<typename Body_, typename Closure>
	static bool call_impl(Closure&& closure, decltype(&Body_::can_split)) {
		auto begin = hpx::util::get<0>(closure);
		auto end = hpx::util::get<1>(closure);

		return Body::can_split(begin, end);
	}

	template<typename Body_, typename Closure>
	static bool call_impl(Closure&& closure, ...) {
		auto begin = hpx::util::get<0>(closure);
		auto end = hpx::util::get<1>(closure);

		// this decision is arbitrary and should be fixed by the Body
		// implementations to give a good estimate for granularity
		return end - begin > 100;
	}

	template<typename Closure>
	static bool call(Closure&& closure) {
		return call_impl<Body>(std::forward<Closure>(closure), nullptr);
	}
};

////////////////////////////////////////////////////////////////////////////////
// PFor Work Item:
//  - Result: std::int64_t
//  - Closure: [std::int64_t,std::int64_t,hpx::util::tuple<ExtraParams...>]
//  - SplitVariant: split range in half, compute halfs
//  - ProcessVariant: cumpute the given range

template<typename Body, typename ... ExtraParams>
struct pfor_split_variant;

template<typename Body, typename ... ExtraParams>
struct pfor_process_variant;

struct pfor_name {
	static const char *name() {
		return "pfor";
	}
};

template<typename Body, typename ... ExtraParams>
using pfor_work =
allscale::work_item_description<
void,
pfor_name,
allscale::no_serialization,
pfor_split_variant<Body,ExtraParams...>,
pfor_process_variant<Body,ExtraParams...>
>;




template<typename Body, typename ... ExtraParams>
struct pfor_split_variant {
	static constexpr bool valid = true;
	using result_type = void;

	// It spawns two new tasks, processing each half the iterations
	template<typename Closure>
	static allscale::treeture<void> execute(Closure const& closure) {
		auto begin = hpx::util::get<0>(closure);
		auto end = hpx::util::get<1>(closure);
		auto extra = hpx::util::get<2>(closure);

		// check whether there are iterations left
		if (begin >= end)
			return allscale::make_ready_treeture();

		// compute the middle
		auto mid = begin + (end - begin) / 2;

		// spawn two new sub-tasks
		return allscale::runtime::treeture_combine(
				allscale::spawn<pfor_work<Body, ExtraParams...>>(begin, mid,
						extra),
				allscale::spawn<pfor_work<Body, ExtraParams...>>(mid, end,
						extra));
	}
};

template<typename Body, typename ... ExtraParams>
struct pfor_process_variant {
	static constexpr bool valid = true;
	using result_type = void;

	static const char* name() {
		return "pfor_process_variant";
	}

	// Execute for serial
	template<typename Closure>
	static allscale::treeture<void> execute(Closure const& closure) {
		auto begin = hpx::util::get<0>(closure);
		auto end = hpx::util::get<1>(closure);
		auto extra = hpx::util::get<2>(closure);

		// get a body instance
		Body body;

		// do some computation
		for (auto i = begin; i < end; ++i) {
			body(i, extra);
		}

		// done
		return allscale::make_ready_treeture();
	}
};

template<typename Body, typename ... ExtraParams>
pfor_loop_handle pfor(int a, int b, const ExtraParams& ... params) {
	return allscale::spawn_first<pfor_work<Body, ExtraParams...>>(a, b,
			hpx::util::make_tuple(params...));
}

////////////////////////////////////////////////////////////////////////////////
// PFor Work Item with fine-grained synchronization (simple)
//  - Result: std::int64_t
//  - Closure: [std::int64_t,std::int64_t,hpx::util::tuple<ExtraParams...>]
//  - SplitVariant: split range in half, compute halfs
//  - ProcessVariant: cumpute the given range

template<typename Body, typename ... ExtraParams>
struct pfor_neighbor_sync_split_variant;

template<typename Body, typename ... ExtraParams>
struct pfor_neighbor_sync_process_variant;

struct pfor_neighbor_sync_name {
	static const char *name() {
		return "pfor_neighbor_sync";
	}
};

template<typename Body, typename ... ExtraParams>
using pfor_neighbor_sync_work =
allscale::work_item_description<
void,
pfor_name,
allscale::do_serialization,
pfor_neighbor_sync_split_variant<Body,ExtraParams...>,
pfor_neighbor_sync_process_variant<Body,ExtraParams...>,
pfor_can_split<Body>
>;



template<typename Body, typename ... ExtraParams>
using pfor_neighbor_sync_work_with_data_items =
allscale::work_item_description<
void,
pfor_name,
allscale::do_serialization,
allscale::do_use_data_items<grid_data_item_descr>,
pfor_neighbor_sync_split_variant<Body,ExtraParams...>,
pfor_neighbor_sync_process_variant<Body,ExtraParams...>,
pfor_can_split<Body>
>;






template<typename Body, typename ... ExtraParams>
struct pfor_neighbor_sync_split_variant {
	static constexpr bool valid = true;
	using result_type = void;

	// It spawns two new tasks, processing each half the iterations
	template<typename Closure>
	static allscale::treeture<void> execute(Closure const& closure) {
		// extract parameters
		auto begin = hpx::util::get<0>(closure);
		auto end = hpx::util::get<1>(closure);
		auto extra = hpx::util::get<2>(closure);

		// extract the dependencies
		auto deps = hpx::util::get<3>(closure);

		// check whether there are iterations left
		if (begin >= end)
			return allscale::make_ready_treeture();

		// compute the middle
		auto mid = begin + (end - begin) / 2;
		//std::cout<<"splitting"<<std::endl;
//        std::cout<<"SPLIT VARIANT executing, mid is: " << mid << std::endl;
		// refine dependencies
		auto dl = hpx::util::get<0>(deps);
		auto dc = hpx::util::get<1>(deps);
		auto dr = hpx::util::get<2>(deps);

		// refine dependencies
		auto dlr = dl.get_right_child();
		auto dcl = dc.get_left_child();
		auto dcr = dc.get_right_child();
		auto drl = dr.get_left_child();

		// spawn two new sub-tasks
//        std::cout<<"SPAWNING 2 new work items: " << begin<<" to " << mid << " and " << mid << " to " << end << std::endl;

		auto left = allscale::spawn<
				pfor_neighbor_sync_work<Body, ExtraParams...>>(begin, mid,
				extra, hpx::util::make_tuple(dlr, dcl, dcr));
		auto right = allscale::spawn<
				pfor_neighbor_sync_work<Body, ExtraParams...>>(mid, end, extra,
				hpx::util::make_tuple(dcl, dcr, drl));

		return allscale::treeture<void>(
				hpx::when_all(left.get_future(), right.get_future(),
						dl.get_future(), dc.get_future(), dr.get_future()));
	}
};

template<typename Body, typename ... ExtraParams>
struct pfor_neighbor_sync_process_variant {
	static constexpr bool valid = true;
	using result_type = void;

	static const char* name() {
		return "pfor_process_variant";
	}
   
    template<typename Closure>
    static std::vector<hpx::id_type> requires(Closure const& closure){
        std::cout<< "Call to requires implementation"<<std::endl;
        std::vector<hpx::id_type> result;
        result.push_back(hpx::find_here());
        return result;
    }

	// Execute for serial
	template<typename Closure>
	static allscale::treeture<result_type> execute(Closure const& closure) {
		// extract the dependencies
		auto deps = hpx::util::get<3>(closure);

		// refine dependencies
		auto dl = hpx::util::get<0>(deps);
		auto dc = hpx::util::get<1>(deps);
		auto dr = hpx::util::get<2>(deps);

		return allscale::treeture<void>(
				hpx::dataflow(hpx::launch::sync,
						hpx::util::unwrapped([closure]()
						{
								// extract parameters
								auto begin = hpx::util::get<0>(closure);
								auto end = hpx::util::get<1>(closure);

								auto all_params = hpx::util::get<2>(closure);
								hpx::naming::id_type dms = hpx::util::get<2>(all_params);
								grid_region_type search_region(begin, end);
								grid_test_requirement tr(search_region);
								//std::cout<< " dms info : " << hpx::naming::get_locality_id_from_id(dms) << std::endl;
								using action_type = typename allscale::components::data_item_manager_server::locate_async_action<grid_data_item_descr>;
								using fut_type = decltype(hpx::async<action_type>(dms,tr));
								std::vector<fut_type> results;
								results.push_back(std::move(hpx::async<action_type>(dms,tr)));
								using get_right = typename allscale::components::data_item_manager_server::get_right_neighbor_action;
								using get_left = typename allscale::components::data_item_manager_server::get_left_neighbor_action;
								using acquire_action = typename allscale::components::data_item_manager_server::acquire_fragment_async_action<grid_data_item_descr>;
								auto left = hpx::async<get_left>(dms).get();
								auto right = hpx::async<get_right>(dms).get();

								if(left) {
									results.push_back(std::move(hpx::async<action_type>(left,tr)));
								}
								if(right) {
									results.push_back(std::move(hpx::async<action_type>(right,tr)));
								}
								std::vector<std::pair<grid_region_type,hpx::id_type>> shopping_list;
								for(auto& el : results) {
									auto res = el.get();
									for(auto& bla : res) {
										shopping_list.push_back(bla);
									}
								}

								std::vector<grid_fragment> writable_fragments;
								std::vector<grid_fragment> readonly_fragments;

								for( auto& el : shopping_list){
									if(hpx::naming::get_locality_id_from_id(el.second) == hpx::get_locality_id())
									{
										writable_fragments.push_back(hpx::async<acquire_action>(el.second,el.first).get());
									}
									else{
										readonly_fragments.push_back(hpx::async<acquire_action>(el.second,el.first).get());
									}
								}

								 auto extra = hpx::util::make_tuple(hpx::util::get<0>(all_params),hpx::util::get<1>(all_params),
										 writable_fragments,
										 readonly_fragments);
								 // get a body instance
								 Body body;
								 // do some computation

//								 std::cout<<"processing on locality: [" << hpx::get_locality_id()<<"]"<<std::endl;
								 for(auto i = begin; i<end; i++) {
								 body(i,extra);
								 }


							}), dl.get_future(), dc.get_future(),
						dr.get_future()));
	}
};

template<typename Body, typename ... ExtraParams>
pfor_loop_handle pfor_neighbor_sync(const pfor_loop_handle& loop, int a, int b,
		const ExtraParams& ... params) {
	allscale::treeture<void> done = allscale::make_ready_treeture();
	return allscale::spawn_first<pfor_neighbor_sync_work<Body, ExtraParams...>>(
			a, b,                                                   // the range
			hpx::util::make_tuple(params...),             // the body parameters
			hpx::util::make_tuple(done, loop.get_treeture(), done) // initial dependencies
					);
}


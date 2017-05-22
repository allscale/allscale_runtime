#include <allscale/no_split.hpp>
#include <allscale/no_serialization.hpp>
#include <allscale/do_serialization.hpp>
#include <allscale/treeture.hpp>
#include <allscale/spawn.hpp>
#include <allscale/scheduler.hpp>
#include <allscale/work_item_description.hpp>

#include <allscale/runtime.hpp>

#include <hpx/include/unordered_map.hpp>
#include <hpx/include/performance_counters.hpp>

#include <unistd.h>

#include "pfor_with_data_items.hpp"

#include <allscale/data_item.hpp>
#include <allscale/treeture.hpp>
#include <allscale/data_item_description.hpp>
#include <allscale/region.hpp>
#include <allscale/fragment.hpp>

std::vector<std::shared_ptr<my_data_item_manager_server>> dms;

static const int DEFAULT_SIZE = 128 * 1024 * 1024;

std::int64_t local_n;
std::int64_t locality_id;
std::int64_t local_begin;
std::int64_t local_end;

struct simple_stencil_body {
//	void operator()(std::int64_t i,
//			const hpx::util::tuple<std::int64_t, std::int64_t,
//					std::shared_ptr<test_data_item>,
//					std::shared_ptr<test_data_item>>& params) const {
	void operator()(std::int64_t i,
			const hpx::util::tuple<std::int64_t, std::int64_t,
					std::vector<grid_fragment>, std::vector<grid_fragment>>& params) const {
		// extract parameters
		auto t = hpx::util::get<0>(params);
		auto n = hpx::util::get<1>(params);
		auto writable_fragments = hpx::util::get<2>(params);
		auto readonly_fragments = hpx::util::get<3>(params);
		bool can_write = false;
		if (writable_fragments.size() == 0) {
//			std::cout   << "processing on i= " << i << " n= " << n
//					<< "no writable fragments for this process action, this is a problem..."
//					<< std::endl;
		}
		if (readonly_fragments.size() == 0) {
//			std::cout << "processing on i= " << i << " n= " << n
//					<< "no readonly fragments for this process action, this is a problem..."
//					<< std::endl;
		}
		for (auto& el : writable_fragments) {
			if (i >= el.region_.begin_ && i <= el.region_.end_) {
				can_write = true;
			}
//			std::cout << "writable region: " << el.region_.to_string()
//					<< std::endl;
		}
		if (!can_write) {
			std::cout << "processing but i=" << i
					<< " does not lie in writable region" << std::endl;
//			for (auto& el : writable_fragments) {
//				std::cout << "writable region: " << el.region_.to_string()
//						<< std::endl;
//			}
//			for (auto& el : readonly_fragments) {
//				std::cout << "readonly region: " << el.region_.to_string()
//						<< std::endl;
//			}

		}

		/*
		 auto t = hpx::util::get<0>(params);
		 auto n = hpx::util::get<1>(params);
		 auto td_one = (std::shared_ptr<test_data_item>) hpx::util::get<2>(
		 params);
		 auto td_two = (std::shared_ptr<test_data_item>) hpx::util::get<3>(
		 params);
		 auto a = ((*td_one).fragment_.ptr_);
		 auto b = ((*td_two).fragment_.ptr_);

		 HPX_ASSERT(i < n);

		 HPX_ASSERT(i < (*a).size());
		 HPX_ASSERT(i < (*b).size());

		 int* A = (t % 2) ? (*a).data() : (*b).data();
		 int* B = (t % 2) ? (*b).data() : (*a).data();

		 // check current state
		 if ((i > 0 && A[i - 1] != A[i]) || (i < n - 1 && A[i] != A[i + 1])) {
		 std::cout << "Error in synchronization!\n";
		 std::cout << "  for i=" << i << "\n";
		 std::cout << "  A[i-1]=" << A[i - 1] << "\n";
		 std::cout << "  A[ i ]=" << A[i] << "\n";
		 std::cout << "  A[i+1]=" << A[i + 1] << "\n";
		 //exit(42);
		 }

		 // update B
		 //std::cout<<"processing on loc: " << hpx::get_locality_id() << " " << B[i]<< " " << A[i] << std::endl;
		 grid_region_type search_region(i - 1, i + 1);
		 grid_test_requirement tr(search_region);


		 for (auto& fut : dms.locate<grid_data_item_descr>(tr)) {
		 fut.then(
		 [](auto f) -> int
		 {
		 std::cout<< "BBBBBBBBBBBBBBBBBBBBBBBBB" << f.get().size() << std::endl;;
		 return 22;
		 });
		 }



		 B[i] = A[i] + 1;

		 */
		//std::cout<<"processing on loc: " << hpx::get_locality_id() << " " << B[i]<< " " << A[i] << std::endl;
	}
};

int hpx_main(int argc, char **argv) {
//
//	// start allscale scheduler ...
//	allscale::scheduler::run(hpx::get_locality_id());
//
//	std::int64_t n = argc >= 2 ? std::stoi(std::string(argv[1])) : DEFAULT_SIZE;
//	std::int64_t steps = argc >= 3 ? std::stoi(std::string(argv[2])) : 1000;
//	std::int64_t iters = argc >= 4 ? std::stoi(std::string(argv[3])) : 1;
//
//	std::vector<hpx::naming::id_type> localities = hpx::find_all_localities();
//
//	if (hpx::get_locality_id() == 0) {
//		for (auto& loc : localities) {
//			auto tmp = std::make_shared<my_data_item_manager_server>(
//					my_data_item_manager_server(loc));
//			dms.push_back(tmp);
//			locality_id = hpx::naming::get_locality_id_from_id((*tmp).get_id());
//			local_n = n / localities.size();
//			local_begin = 0 + (locality_id * local_n);
//			local_end = local_begin + local_n - 1;
//			auto vec = std::make_shared<std::vector<int>>(
//					std::vector<int>(local_n, 0));
//			grid_region_type gr(local_begin, local_end);
//			grid_fragment grid_frag(gr, vec);
//			grid_data_item_descr grid_descr(gr, grid_frag, nullptr);
//
//			(*dms[locality_id]).create_data_item < grid_data_item_descr
//					> (grid_descr);
//
//			//auto ted = (*tmp).create_data_item<grid_data_item_descr>(grid_descr);
//			//	auto ted2 =	(*tmp).create_data_item<grid_data_item_descr>(grid_descr);
//		}
//		//std::cout<<"first dms " <<  dms[0].get_id()<<std::endl;
//		using set_right = typename allscale::components::data_item_manager_server::set_right_neighbor_action;
//		hpx::async<set_right>((*dms[0]).get_id(), (*dms[1]).get_id());
//		using set_left = typename allscale::components::data_item_manager_server::set_left_neighbor_action;
//		hpx::async<set_left>((*dms[1]).get_id(), (*dms[0]).get_id());
//
//	}
//
//	// initialize the data array
//	//dataA.resize(n, 0);
//	//dataB.resize(n, 0);
//
//	auto a = std::make_shared<std::vector<int>>(std::vector<int>(n, 0));
//	auto b = std::make_shared<std::vector<int>>(std::vector<int>(n, 0));
//
//	descr test_descr;
//	my_region test_region(2);
//	//    my_fragment frag(test_region,7);
//
//	my_fragment frag(test_region, a);
////	auto td = std::make_shared<test_data_item>(
////			test_data_item(localities[0], test_descr, frag));
//
//	descr test_descr2;
//	my_region test_region2(2);
//	my_fragment frag2(test_region, b);
//	//my_fragment frag2(test_region,7);
////	auto td2 = std::make_shared<test_data_item>(
////			test_data_item(localities[0], test_descr2, frag2));
//
//	//test_data_item td2(localities[0], test_descr2, frag2);
//
//	if (hpx::get_locality_id() == 0) {
//		for (int i = 0; i < iters; i++) {
//			std::cout << "Starting " << steps << "x pfor(0.." << n << "), "
//					<< "Iter: " << i << "\n";
//			hpx::util::high_resolution_timer t;
//
//			{
//				pfor_loop_handle last;
//				for (int t = 0; t < steps; t++) {
//					last = pfor_neighbor_sync<simple_stencil_body>(last, 0, n,
//							t, n, (*dms[0]).get_id(), n, n);
//
////					last = pfor_neighbor_sync<simple_stencil_body>(last, 0, n, t, n, td, td2);
//				}
//			}
//
//			auto elapsed = t.elapsed_microseconds();
//			std::cout << "pfor(0.." << n << ") taking " << elapsed
//					<< " microseconds. Iter: " << i << "\n";
//		}
//		allscale::scheduler::stop();
//
////	 auto k = (std::vector<int>) *(td2->fragment_.ptr_);
////
////	 for (auto el : k) {
////	 std::cout << el << " ";
////	 }
////	 std::cout << std::endl;
//	}
//
//	//	auto k = (std::vector<int>) * (td.fragment_.ptr_);
//	//	for(auto el : k){
//	//		std::cout<<el<<" "<<std::endl;
//	//	}
//
    return hpx::finalize();
}

int main(int argc, char **argv) {
	return hpx::init(argc, argv);
}


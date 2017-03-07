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



static const int DEFAULT_SIZE = 128 * 1024 * 1024;

static std::vector<int> dataA;
static std::vector<int> dataB;

struct simple_stencil_body {
	void operator()(std::int64_t i,
			const hpx::util::tuple<std::int64_t, std::int64_t, std::shared_ptr<test_data_item>,
					std::shared_ptr<test_data_item>>& params) const {
		// extract parameters
		auto t = hpx::util::get<0>(params);
		auto n = hpx::util::get<1>(params);

		auto td_one = (std::shared_ptr<test_data_item>) hpx::util::get<2>(params);
		auto td_two = (std::shared_ptr<test_data_item>) hpx::util::get<3>(params);

		auto a = ((*td_one).fragment_.ptr_);
		auto b = ((*td_two).fragment_.ptr_);

		//auto b = (std::vector<int>*)  ((*td_two).fragment_.ptr_);


		//auto td_two = hpx::util::get<3>(params);
		//auto b = (std::vector<int>) * (td_two.fragment_.ptr_);
        //auto res = (std::int64_t) *(td_one.fragment_.ptr_);
        //res++;
		//( * ((*td_one).fragment_.ptr_))++;
        //std::cout<< ( * ((*td_one).fragment_.ptr_)) <<std::endl;
		//(*a)[i]++;
		//std::cout<< (*a)[i] << std::endl;
		HPX_ASSERT(i < n);

		HPX_ASSERT(i < (*a).size());
		HPX_ASSERT(i < (*b).size());

		//HPX_ASSERT(i < a.size());
		//HPX_ASSERT(i < b.size());

		//a[0] = a[i]+1;
		//std::cout<<a[i];
		// figure out in which way to move the data
		/*
		int* A = (t % 2) ? dataA.data() : dataB.data();
		int* B = (t % 2) ? dataB.data() : dataA.data();
		*/


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
		B[i] = A[i] + 1;

	}
};

int hpx_main(int argc, char **argv) {

	std::cout<<"fick deine mutdder\n";

	// start allscale scheduler ...
	allscale::scheduler::run(hpx::get_locality_id());

	std::int64_t n = argc >= 2 ? std::stoi(std::string(argv[1])) : DEFAULT_SIZE;
	std::int64_t steps = argc >= 3 ? std::stoi(std::string(argv[2])) : 1000;
	std::int64_t iters = argc >= 4 ? std::stoi(std::string(argv[3])) : 1;

	// initialize the data array
	dataA.resize(n, 0);
	dataB.resize(n, 0);

	auto a = std::make_shared<std::vector<int>>(std::vector<int> (n, 0));
	auto b = std::make_shared<std::vector<int>>(std::vector<int> (n, 0));



	std::vector<hpx::naming::id_type> localities = hpx::find_all_localities();

	descr test_descr;
	my_region test_region(2);
//    my_fragment frag(test_region,7);

	my_fragment frag(test_region, a);
	auto td = std::make_shared<test_data_item>(test_data_item(localities[0], test_descr, frag));

	descr test_descr2;
	my_region test_region2(2);
	my_fragment frag2(test_region, b);
    //my_fragment frag2(test_region,7);
	auto td2 = std::make_shared<test_data_item>(test_data_item(localities[0], test_descr2, frag2));

	//test_data_item td2(localities[0], test_descr2, frag2);

	if (hpx::get_locality_id() == 0) {
		for (int i = 0; i < iters; i++) {
			std::cout << "Starting " << steps << "x pfor(0.." << n << "), "
					<< "Iter: " << i << "\n";
			hpx::util::high_resolution_timer t;

			{
				pfor_loop_handle last;
				for (int t = 0; t < steps; t++) {
					std::cout<<"calling pfor_neighbor_sync";
					last = pfor_neighbor_sync<simple_stencil_body>(last, 0, n,
							t, n, td, td2);
				}
			}

			auto elapsed = t.elapsed_microseconds();
			std::cout << "pfor(0.." << n << ") taking " << elapsed
					<< " microseconds. Iter: " << i << "\n";
		}
		allscale::scheduler::stop();
	}

//	auto k = (std::vector<int>) * (td.fragment_.ptr_);
//	for(auto el : k){
//		std::cout<<el<<" "<<std::endl;
//	}


	return hpx::finalize();
}

int main(int argc, char **argv) {
	return hpx::init(argc, argv);
}


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

//#include "pfor_with_data_items.hpp"
#include "pfor.hpp"

#include <allscale/data_item.hpp>
#include <allscale/treeture.hpp>
#include <allscale/data_item_description.hpp>
#include <allscale/region.hpp>
#include <allscale/fragment.hpp>


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




//ALLSCALE_REGISTER_FRAGMENT_TYPE(grid_region_type,value_type);









std::vector<std::shared_ptr<my_data_item_manager_server>> dms;

static const int DEFAULT_SIZE = 128 * 1024 * 1024;

std::int64_t local_n;
std::int64_t locality_id;
std::int64_t local_begin;
std::int64_t local_end;



struct simple_stencil_body {
    void operator()(std::int64_t i, const hpx::util::tuple<std::int64_t,std::int64_t,hpx::id_type, hpx::id_type, hpx::id_type>& params) const {
        // extract parameters
        auto t = hpx::util::get<0>(params);
        auto n = hpx::util::get<1>(params);
        auto data_item_a_id = hpx::util::get<2>(params);

        auto server_0 =  hpx::util::get<3>(params);
        auto server_1 =  hpx::util::get<4>(params);
    	grid_region_type reg(0, 100000);

    	using acquire_action = typename allscale::components::data_item_manager_server::acquire_fragment_async_action<grid_data_item_descr>;


        if(hpx::get_locality_id()==0){
        	std::cout<<"not on main \n";
            std::cout<<"loc from item : "<< hpx::naming::get_locality_id_from_id(data_item_a_id) <<" loc:" <<  hpx::get_locality_id()<<std::endl;
            
            
            hpx::future<grid_fragment> fut = hpx::async<acquire_action>(server_0,data_item_a_id,reg);
            // hpx::future<grid_fragment> fut = hpx::async<acquire_action>(server_0,data_item_a_id,reg);
            grid_fragment frag = fut.get();
			//std::cout<<"bullshit" << std::endl;
            //

		    (*frag.ptr_)[i] = i;

        }




        if(hpx::get_locality_id()==1){
        	std::cout<<"not on main \n";
            std::cout<<"loc from item : "<< hpx::naming::get_locality_id_from_id(data_item_a_id) <<" loc:" <<  hpx::get_locality_id()<<std::endl;
            
            
            hpx::future<grid_fragment> fut = hpx::async<acquire_action>(server_1,data_item_a_id,reg);
            // hpx::future<grid_fragment> fut = hpx::async<acquire_action>(server_0,data_item_a_id,reg);
            grid_fragment frag = fut.get();
			//std::cout<<"bullshit" << std::endl;
            //

		    (*frag.ptr_)[i] = i;

        }


//        HPX_ASSERT(i < n);
//        HPX_ASSERT(i < dataA.size());
//        HPX_ASSERT(i < dataB.size());
//
//        // figure out in which way to move the data
//        int* A = (t%2) ? dataA.data() : dataB.data();
//        int* B = (t%2) ? dataB.data() : dataA.data();
//
//        // check current state
//        if ((i > 0 && A[i-1] != A[i]) || (i < n-1 && A[i] != A[i+1])) {
//                std::cout << "Error in synchronization!\n";
//                std::cout << "  for i=" << i << "\n";
//                std::cout << "  A[i-1]=" << A[i-1] << "\n";
//                std::cout << "  A[ i ]=" << A[ i ] << "\n";
//                std::cout << "  A[i+1]=" << A[i+1] << "\n";
//                exit(42);
//        }
//        // update B
//        B[i] = A[i] + 1;
    }
};







bool init_data_item_manager_server() {
	std::vector<hpx::naming::id_type> localities = hpx::find_all_localities();
	if (hpx::get_locality_id() == 0) {
		std::vector<hpx::naming::id_type> server_ids;
		for (hpx::naming::id_type const& node : localities) {
			auto tmp = std::make_shared<my_data_item_manager_server>(my_data_item_manager_server(node));
			dms.push_back(tmp);
		}
	}
	return true;
}

int hpx_main(int argc, char **argv) {

	// start allscale scheduler ...
	allscale::scheduler::run(hpx::get_locality_id());

	std::int64_t n = argc >= 2 ? std::stoi(std::string(argv[1])) : DEFAULT_SIZE;
	std::int64_t steps = argc >= 3 ? std::stoi(std::string(argv[2])) : 1000;
	std::int64_t iters = argc >= 4 ? std::stoi(std::string(argv[3])) : 1;

	std::vector<hpx::naming::id_type> localities = hpx::find_all_localities();
	if(init_data_item_manager_server()){
		std::cout<<"init ok"<<std::endl;
	}
	if (hpx::get_locality_id() == 0) {
		std::cout<<"on main loc "<<std::endl;
		auto server_0 = (*dms[0]);
		auto server_1 = (*dms[1]);

		auto data_item_a = server_0.create_data_item<grid_data_item_descr>();
		auto data_item_b = server_0.create_data_item<grid_data_item_descr>();
		auto data_item_c = server_1.create_data_item<grid_data_item_descr>();
		std::cout<<"created data_item_a" << data_item_a << std::endl;
		std::cout<<"created data_item_b " << data_item_b << std::endl;
		std::cout<<"created data_item_c" << data_item_c << std::endl;
		using create_fragment_async_action_type= typename allscale::components::data_item_manager_server::create_fragment_async_action
		<
			grid_data_item_descr,grid_region_type,value_type
		>;

		grid_region_type gr(0,n);

		value_type vec(n,-1);
		hpx::async<create_fragment_async_action_type>(server_0.get_id(),data_item_a, gr,gr,vec);
		hpx::async<create_fragment_async_action_type>(server_0.get_id(),data_item_b, gr,gr,vec);

		hpx::async<create_fragment_async_action_type>(server_1.get_id(),data_item_c, gr,gr,vec);

		 for(int i=0; i<iters; i++) {
		            std::cout << "Starting " << steps << "x pfor(0.." << n << "), " << "Iter: " << i << "\n";
		            hpx::util::high_resolution_timer t;

		            {
		            	  pfor_loop_handle last;
		            	       for(int t=0; t<steps; t++) {

		            	    	   last = pfor_neighbor_sync<simple_stencil_body>(last,0,n,t,n,data_item_a,server_0.get_id(),server_1.get_id());
		            	       }
		            }
		 }



    	grid_region_type reg(0, 100000);
    	using acquire_action = typename allscale::components::data_item_manager_server::acquire_fragment_async_action<grid_data_item_descr>;
        hpx::future<grid_fragment> fut = hpx::async<acquire_action>(server_0.get_id(),data_item_a,reg);
        grid_fragment frag = fut.get();

        for(int i = 0; i < 100000; ++i){
            std::cout << (*frag.ptr_)[i] << std::endl;
        }










		 allscale::scheduler::stop();




		//pfor_neighbor_sync<simple_stencil_body>(last, 0, n, 0, n, server_1.get_id(), n, n);
        //last = pfor_neighbor_sync<simple_stencil_body>(last,0,n,t,n);


		/*
		grid_region_type search_region(0, 20);
		grid_test_requirement tr(search_region);
		using locate_action = typename allscale::components::data_item_manager_server::locate_async_action<grid_data_item_descr>;

		auto res = hpx::async<locate_action>(server_0.get_id(), data_item_a,tr).get();
		std::vector<std::pair<grid_region_type,hpx::id_type>> shopping_list;
		for (auto& el : res) {
			shopping_list.push_back(el);
			std::cout<< el.first.to_string() << " " << el.second << std::endl;

			//std::cout<< el.first.to_string() << " " << hpx::naming::get_locality_id_from_id(el.second) << std::endl;
		}*/



//		for(auto & server_ptr : dms){
//			auto server = *server_ptr;
//			auto td_id = server.create_data_item<grid_data_item_descr>();
//			std::cout<<"created data_item " << td_id << std::endl;
//

//	//			for (int k = 0; k < 10; ++k) {
//	//				/*
//	//
//	//				//grid_fragment frag(gr, a);
//	//

//		}
	}
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
////		using set_right = typename allscale::components::data_item_manager_server::set_right_neighbor_action;
////		hpx::async<set_right>((*dms[0]).get_id(), (*dms[1]).get_id());
////		using set_left = typename allscale::components::data_item_manager_server::set_left_neighbor_action;
////		hpx::async<set_left>((*dms[1]).get_id(), (*dms[0]).get_id());
//
//	}

	// initialize the data array
	//dataA.resize(n, 0);
	//dataB.resize(n, 0);
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
	//my_fragment frag2(test_region,7);
//	auto td2 = std::make_shared<test_data_item>(
//			test_data_item(localities[0], test_descr2, frag2));

	//test_data_item td2(localities[0], test_descr2, frag2);

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

//	 auto k = (std::vector<int>) *(td2->fragment_.ptr_);
//
//	 for (auto el : k) {
//	 std::cout << el << " ";
//	 }
//	 std::cout << std::endl;


	//	auto k = (std::vector<int>) * (td.fragment_.ptr_);
	//	for(auto el : k){
	//		std::cout<<el<<" "<<std::endl;
	//	}

    return hpx::finalize();
}

int main(int argc, char **argv) {
	return hpx::init(argc, argv);
}


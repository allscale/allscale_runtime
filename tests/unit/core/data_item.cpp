#include <iostream>
#include <array>
#include <string>
#include <type_traits>
#include <typeinfo>

#include <allscale/data_item.hpp>
#include <allscale/treeture.hpp>
#include <allscale/data_item_description.hpp>
#include <allscale/region.hpp>
#include <allscale/fragment.hpp>
#include <hpx/config.hpp>
#include <hpx/hpx_main.hpp>
#include <hpx/util/high_resolution_timer.hpp>

using my_region = allscale::region<int>;
using my_fragment = allscale::fragment<my_region,int>;
using descr = allscale::data_item_description<my_region,my_fragment,int>;
using test_item = allscale::data_item<descr>;

ALLSCALE_REGISTER_DATA_ITEM_TYPE(descr);

int some_global_function(test_item  td) {
	return *(td.fragment_.ptr_) * 7;
}

HPX_PLAIN_ACTION(some_global_function, some_global_action);

bool test_action_invocation_distributed() {
	auto locs = hpx::find_all_localities();
	if (locs.size() < 2) {
		std::cout
				<< "Distributed test must be run with more than one localities, set hpx::localities >=2"
				<< std::endl;
		return false;
	}
	//create data item on loc 1
	descr test_descr;
	my_region test_region(2);
	my_fragment frag(test_region, 6);
	some_global_action act;

	return 42==act(locs[1], test_item(locs[0],test_descr,frag));;
}

bool test_action_invocation() {
	descr test_descr;
	my_region test_region(2);
	my_fragment frag(test_region, 6);
	auto td = test_item(hpx::find_here(), test_descr, frag);
	some_global_action act;     // define an instance of some_global_action
	return 42 == act(hpx::find_here(), td);
}

bool test_creation() {

	descr test_descr;
	my_region test_region(2);
	my_fragment frag(test_region, 2);
	test_item td(hpx::find_here(), test_descr, frag);
	return td.valid();
}

int hpx_main(int argc, char* argv[]) {
	if (hpx::get_locality_id() == 0) {

		std::cout << "Running Tests on locality: " << hpx::get_locality_id()
				<< std::endl;
		std::cout << ""
				<< (test_creation() ? "PASSED" : "FAILED") << std::endl;
		std::cout << ""
				<< (test_action_invocation() ? "PASSED" : "FAILED")
				<< std::endl;
		std::cout << ""
				<< (test_action_invocation_distributed() ? "PASSED" : "FAILED")
				<< std::endl;

	}
	return hpx::finalize();

}

int main(int argc, char* argv[])
{
	return hpx::init(argc, argv);
}


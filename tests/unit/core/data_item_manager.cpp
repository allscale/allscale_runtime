#include "allscale/api/user/data/scalar.h"
#include <allscale/data_item_reference.hpp>
#include <allscale/data_item_server_network.hpp>
#include <algorithm>

#include <hpx/hpx_main.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/config.hpp>
#include <hpx/include/components.hpp>

#include <hpx/runtime/serialization/serialize.hpp>
#include <hpx/runtime/serialization/input_archive.hpp>
#include <hpx/runtime/serialization/output_archive.hpp>
#include <hpx/runtime/serialization/vector.hpp>
#define EXPECT_EQ(X,Y)  X==Y
#define EXPECT_NE(X,Y)  X!=Y


using namespace allscale::api::user::data;
using namespace allscale;
HPX_REGISTER_COMPONENT_MODULE();


using data_item_type = Scalar<int>;
REGISTER_DATAITEMREFERENCE(data_item_type);


void test_scalar_data_item_reference() {

    typedef typename server::data_item_reference<data_item_type> data_item_reference_type;

    if (hpx::get_locality_id() == 0) {
        std::cout << "on locality " << hpx::get_locality_id() << " running test"
                << std::endl;
        std::vector < hpx::id_type > localities = hpx::find_all_localities();

        for (auto& loc : localities) {
            data_item_reference<data_item_type> ref(
                    hpx::components::new_ < data_item_reference_type > (loc));
            //ref.print();
            ref.set_value(133);
            //ref.print();

        }
    }
}

void test_data_item_server_network_creation(){



}



int hpx_main(int argc, char* argv[]) {
    test_scalar_data_item_reference();
    //std::cout<<std::endl;
    //test_grid_data_item_server_create();
    //std::cout<<std::endl;
    //test_grid_data_item_server_get();
 //   test_grid_data_item_server_network_creation();
    return hpx::finalize();
}

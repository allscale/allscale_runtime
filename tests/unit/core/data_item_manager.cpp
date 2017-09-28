#include "allscale/api/user/data/scalar.h"
#include "allscale/api/user/data/grid.h"
#include <allscale/data_item_reference.hpp>
#include <allscale/data_item_server.hpp>
#include <allscale/data_item_server_network.hpp>
#include <allscale/data_item_manager.hpp>
#include <allscale/data_item_requirement.hpp>
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
REGISTER_DATAITEMSERVER_DECLARATION(data_item_type);
REGISTER_DATAITEMSERVER(data_item_type);





using data_item_type_grid = Grid<int,1>;
REGISTER_DATAITEMSERVER_DECLARATION(data_item_type_grid);
REGISTER_DATAITEMSERVER(data_item_type_grid);








void test_scalar_data_item_reference() {

   // typedef typename server::data_item_reference<data_item_type> data_item_reference_type;

    if (hpx::get_locality_id() == 0) {
        std::vector < hpx::id_type > localities = hpx::find_all_localities();
/*
        for (auto& loc : localities) {
            data_item_reference<data_item_type> ref(
                    hpx::components::new_ < data_item_reference_type > (loc));
            //ref.print();
            ref.set_value(133);
            //ref.print();

        }
    */
        }

}


void test_data_item_server_creation(){




}



template <typename DataItemType, typename ... Args>
auto simulate_data_item_manager_create_and_get(const Args& ... args){
    
    std::vector<allscale::data_item_server<DataItemType>> result;
    //SERIALIZE STUFF BY HAND FOR NOW
    auto archive = allscale::utils::serialize(args...);
    using buffer_type = std::vector<char>;
    buffer_type buffer;
    buffer = archive.getBuffer();
   

    if (hpx::get_locality_id() == 0) {
        //auto sn = allscale::data_item_manager::create_server_network<DataItemType>();
        auto dataRef = allscale::data_item_manager::create<DataItemType>(args...);

        // acquire small lease
        auto req = allscale::createDataItemRequirement(dataRef, GridRegion<1>(100,150), access_mode::ReadWrite); 
        auto lease = allscale::data_item_manager::acquire<DataItemType>(req);


        // acquire bigger lease
        auto req2 = allscale::createDataItemRequirement(dataRef, GridRegion<1>(,5100), access_mode::ReadWrite); 
        auto lease2 = allscale::data_item_manager::acquire<DataItemType>(req2);


	    auto data = allscale::data_item_manager::get(dataRef);
        
        //std::cout<<data[150]<<std::endl;
        //data[5000] = 5;
        //std::cout<<data[5000]<<std::endl;
    }
    /*
    if (hpx::get_locality_id() == 0) {
		//CYCLE THRU LOCALITIES
		std::cout << "on locality " << hpx::get_locality_id() << " running test" << std::endl;
        std::vector < hpx::id_type > localities = hpx::find_all_localities();
		//CREATE DATA ITEM SERVER INSTANCES ON LOCALITIES
		for (auto& loc : localities) {
		    allscale::data_item_server<DataItemType> server(
					hpx::components::new_ < data_item_server_type > (loc));
            auto res = server.create(buffer);
            result.push_back(server);
		}



       allscale::data_item_server_network<DataItemType> sn;
       sn.servers = result;

       for( auto& server : result){
        server.set_network(sn);
       }


        auto data_item_server_name = allscale::data_item_server_name<DataItemType>::name();
        auto res =  hpx::find_all_from_basename(data_item_server_name, 2);
        for(auto& fut : res ){
            typedef typename allscale::server::data_item_server<DataItemType>::print_action action_type;
		    action_type()(fut.get());
        }
      

	}*/
    // auto server = allscale::data_item_manager::getServer<DataItemType>();
    // server.set_servers();
    return result;
}


void test_grid_data_item_server_create_and_get() {
    GridPoint<1> size = 200;
	using data_item_shared_data_type = typename data_item_type_grid::shared_data_type;
    data_item_shared_data_type sharedData(size);
    simulate_data_item_manager_create_and_get<Grid<int,1>>(sharedData);
}





int hpx_main(int argc, char* argv[]) {
    test_scalar_data_item_reference();
   // test_data_item_server_creation();
	test_grid_data_item_server_create_and_get();
    
    //std::cout<<std::endl;
    //test_grid_data_item_server_create();
    //std::cout<<std::endl;
    //test_grid_data_item_server_get();
 //   test_grid_data_item_server_network_creation();
    return hpx::finalize();
}

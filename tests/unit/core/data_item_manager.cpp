#include "allscale/api/user/data/scalar.h"
#include "allscale/api/user/data/grid.h"
#include "allscale/api/user/data/static_grid.h"
#include <allscale/data_item_requirement.hpp>
#include <allscale/data_item_manager.hpp>
#include <allscale/locality.h>
#include <algorithm>

#define EXPECT_EQ(X,Y)  X==Y
#define EXPECT_NE(X,Y)  X!=Y


using namespace allscale::api::user::data;
using namespace allscale::simulator;


bool test_location_manager(){
    if(5!=getNumLocations()){
        return false;
    }
    if(0!=getLocality()){
        return false;    
    }
    setLocality(3);
    if(3!=getLocality()){
        return false;    
    }
    return true;
}


bool test_data_item_manager_scalar(){
    auto dataRef = allscale::data_item_manager::create<Scalar<int>>(); 
    //allscale::data_item_server<Scalar<int>> server = allscale::data_item_server_network<Scalar<int>>::getLocalDataItemServer();
    auto lease = allscale::data_item_manager::acquire(allscale::createDataItemRequirement(dataRef,allscale::api::user::data::detail::ScalarRegion(true),allscale::access_mode::ReadWrite));
    auto data = allscale::data_item_manager::get(dataRef);
    data.set(12);
    allscale::data_item_manager::release(lease);
    allscale::data_item_manager::destroy(dataRef);
    return true;
}

bool test_data_item_manager_grid(){
    GridPoint<1> size = 200;
    auto dataRef = allscale::data_item_manager::create<Grid<int,1>>(size);
    auto lease = allscale::data_item_manager::acquire(allscale::createDataItemRequirement(dataRef,GridRegion<1>(100,150),allscale::access_mode::ReadWrite));
    auto data = allscale::data_item_manager::get(dataRef);
    data[120]=5;
    allscale::data_item_manager::release(lease);
    allscale::data_item_manager::destroy(dataRef);
    return 1;
}

bool test_data_item_manager_static_grid(){
    auto dataRef = allscale::data_item_manager::create<StaticGrid<int,200>>();
    auto lease = allscale::data_item_manager::acquire(allscale::createDataItemRequirement(dataRef,GridRegion<1>(100,150),allscale::access_mode::ReadWrite));
    auto data = allscale::data_item_manager::get(dataRef);
    data[120]=5;
    allscale::data_item_manager::release(lease);
    allscale::data_item_manager::destroy(dataRef);
    return 1;
}

bool test_data_item_manager_multi_server(){
    setLocality(0);
    auto dataRef = allscale::data_item_manager::create<Scalar<int>>(); 
    setLocality(1);
    auto lease = allscale::data_item_manager::acquire(allscale::createDataItemRequirement(dataRef,allscale::api::user::data::detail::ScalarRegion(true),allscale::access_mode::ReadWrite));
    auto data = allscale::data_item_manager::get(dataRef);
    data.set(12);
    allscale::data_item_manager::release(lease);
    setLocality(2);
    allscale::data_item_manager::destroy(dataRef);
    return 1;
}

bool test_data_item_manager_scalar_single_producer_single_consumer(){
    setLocality(0);


    auto dataRef = allscale::data_item_manager::create<Scalar<int>>();

    int* loc1 = nullptr;
    int* loc2 = nullptr;

    // write something to the data item on locality 1
    {
        setLocality(1);

        auto lease = allscale::data_item_manager::acquire(allscale::createDataItemRequirement(dataRef,allscale::api::user::data::detail::ScalarRegion(true),allscale::access_mode::ReadWrite));
        auto data = allscale::data_item_manager::get(dataRef);
        
        loc1 = &data.get();
        data.set(12);
        allscale::data_item_manager::release(lease);
    }

    // and we consume it on locality 2
    {
        setLocality(2);
        
        auto lease = allscale::data_item_manager::acquire(allscale::createDataItemRequirement(dataRef,allscale::api::user::data::detail::ScalarRegion(true),allscale::access_mode::ReadWrite));
        auto data = allscale::data_item_manager::get(dataRef);

        loc2 = &data.get();
        
        std::cout<<"equal ? " << (EXPECT_EQ(12,data.get())) << std::endl;

        allscale::data_item_manager::release(lease);
    }


    std::cout<<"not equal ? " << (EXPECT_NE(loc1,loc2)) << std::endl;
    // and we destroy it on locality 3
    setLocality(3);
    allscale::data_item_manager::destroy(dataRef);
    return 1;
}


int main()
{
    
    std::cout<<test_location_manager()<<std::endl; 
    std::cout<<test_data_item_manager_scalar()<<std::endl; 
    std::cout<<test_data_item_manager_grid()<<std::endl; 
    std::cout<<test_data_item_manager_static_grid()<<std::endl; 
    std::cout<<test_data_item_manager_multi_server()<<std::endl; 
    std::cout<<test_data_item_manager_scalar_single_producer_single_consumer()<<std::endl;
    return 0;

}

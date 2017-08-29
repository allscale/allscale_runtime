#include "allscale/api/user/data/scalar.h"
#include <allscale/data_item_requirement.hpp>
#include <allscale/data_item_manager.hpp>
#include <allscale/locality.h>

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
int main()
{
    
    std::cout<<test_location_manager()<<std::endl; 
    std::cout<<test_data_item_manager_scalar()<<std::endl; 

    return 0;

}

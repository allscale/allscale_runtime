#include "allscale/api/user/data/scalar.h"
#include <allscale/data_item_requirement.hpp>
#include <allscale/data_item_manager.hpp>


using namespace allscale::api::user::data;

int main()
{
    
    auto dataRef = allscale::data_item_manager::create<Scalar<int>>(); 
    //allscale::data_item_server<Scalar<int>> server = allscale::data_item_server_network<Scalar<int>>::getLocalDataItemServer();
    auto lease = allscale::data_item_manager::acquire(allscale::createDataItemRequirement(dataRef,allscale::api::user::data::detail::ScalarRegion(true),allscale::access_mode::ReadWrite));
    return 0;
}



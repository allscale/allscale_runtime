#include <iostream>
#include <array>
#include <string>
#include <type_traits>
#include <typeinfo>
//#include <unordered_map>
#include <allscale/data_item.hpp>
#include <allscale/data_item_description.hpp>
#include <allscale/region.hpp>
#include <allscale/data_item_manager_server.hpp>
#include <allscale/requirement.hpp>

#include <hpx/config.hpp>
#include <hpx/hpx_main.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/util/high_resolution_timer.hpp>



typedef allscale::region<int> my_region;
typedef allscale::data_item_description<my_region,int,int> descr;
typedef allscale::data_item<descr> test_item;

ALLSCALE_REGISTER_DATA_ITEM_TYPE(descr);


typedef allscale::data_item_manager_server my_data_item_manager_server;

typedef std::pair<hpx::naming::id_type, my_data_item_manager_server> loc_server_pair;


ALLSCALE_REGISTER_DATA_ITEM_MANAGER_SERVER_COMPONENT();
/*
template<typename T>
bool check(T first)
{
    if(allscale::traits::is_data_item<T>::value){
        //std::cout<<"allscale::traits::is_treeture<T>::value==TRUE"<<std::endl;
        return true;
    }
    else
    {
        //std::cout<<"allscale::traits::is_treeture<T>::value==FALSE"<<std::endl;
        return false;
    }
}
*/


int hpx_main(int argc, char* argv[])
{

    std::vector< loc_server_pair > dms;
    std::vector<hpx::naming::id_type> localities = hpx::find_all_localities();
    if(hpx::get_locality_id() == 0){
        
        for(hpx::naming::id_type const& node : localities) {
            std::cout << "Cycling thru localities, Locality: " << node << std::endl;
            auto tmp = my_data_item_manager_server(node);
            dms.push_back(std::make_pair(node, tmp));
        
        }



        for(loc_server_pair & server_entry : dms)
        {

            my_region test_region;
            descr test_descr;
            auto test = server_entry.second.create_data_item<descr>(test_descr);
            std::cout<<"test item global id: " << test.get_gid() << std::endl;

        }

    }
    
        
    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    return hpx::init(argc, argv); 
}






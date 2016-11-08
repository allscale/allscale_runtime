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
typedef allscale::requirement<descr> test_requirement;

typedef allscale::data_item_manager_server my_data_item_manager_server;
typedef std::pair<hpx::naming::id_type, my_data_item_manager_server> loc_server_pair;


ALLSCALE_REGISTER_DATA_ITEM_TYPE(descr);
ALLSCALE_REGISTER_DATA_ITEM_MANAGER_SERVER_COMPONENT();


std::vector< loc_server_pair > dms;

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

bool test_creation_of_data_item_manager_server_components(){
    std::vector<hpx::naming::id_type> localities = hpx::find_all_localities();
    if(hpx::get_locality_id() == 0){

        for(hpx::naming::id_type const& node : localities) {
            //std::cout << "Cycling thru localities, Locality: " << node << std::endl;
            auto tmp = my_data_item_manager_server(node);
            dms.push_back(std::make_pair(node, tmp));
        }
    }
    return true;
}


bool test_creation_of_data_items()
{
    for(loc_server_pair & server_entry : dms)
    {
        my_region test_region;
        descr test_descr;
        for(int i = 0; i < 10; ++i){
            auto test = server_entry.second.create_data_item<descr>(test_descr);
             //std::cout<<"test item global id: " << test.get_gid() << std::endl;
        }
    }
    return true;
}




bool test_locate_method()
{
    test_requirement my_req;
    descr test_descr;
    for(loc_server_pair & server_entry : dms)
    {
        server_entry.second.locate<descr>(my_req);
    }
    return true;
}











int hpx_main(int argc, char* argv[])
{
    if(test_creation_of_data_item_manager_server_components()==true){
        std::cout<< "Test(1): Creation of data_item_manager_servers : PASSED"<<std::endl; 
    }
 
    if(test_creation_of_data_items()==true){
        std::cout<< "Test(2): Creation of data_items : PASSED"<<std::endl; 
    }
    
    if(test_locate_method()==true){
        std::cout<< "Test(3): Locate all data_items related to a region of a requirement: PASSED"<<std::endl; 
    }


    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    return hpx::init(argc, argv);
}






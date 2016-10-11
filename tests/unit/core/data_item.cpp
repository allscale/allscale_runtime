#include <iostream>
#include <array>
#include <string>
#include <type_traits>
#include <typeinfo>

#include <allscale/data_item.hpp>
#include <allscale/treeture.hpp>
#include <allscale/data_item_description.hpp>
#include <allscale/region.hpp>

#include <hpx/config.hpp>
#include <hpx/hpx_main.hpp>
#include <hpx/util/high_resolution_timer.hpp>



typedef allscale::region<int> my_region;
typedef allscale::data_item_description<my_region,int,int> descr;
typedef allscale::data_item<descr> test_item;

ALLSCALE_REGISTER_DATA_ITEM_TYPE(descr);


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

int main() 
{

    std::vector<test_item> tvector;

    std::vector<hpx::naming::id_type> localities = 
        hpx::find_all_localities();
    for(hpx::naming::id_type const& node : localities)
    {
//        test_item tmp(node);
  //      tvector.push_back(tmp);

   //     std::cout<<"item:  " << tmp.parent_loc << std::endl;  
    }

    /*
    if(hpx::get_locality_id() == 0)
    {
        std::cout<<"On Locality 0" << "\n";
        test_item tmp(hpx::find_here());
    }
    else
    {
        std::cout<<"On Locality " << hpx::get_locality_id()<< "\n";
    }*/
   
    return 0;
}






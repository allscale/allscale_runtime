#include <iostream>
#include <array>
#include <string>
#include <type_traits>
#include <typeinfo>

#include <allscale/region.hpp>

#include <hpx/config.hpp>
#include <hpx/hpx_main.hpp>
#include <hpx/util/high_resolution_timer.hpp>


typedef allscale::region<int> my_region;

int main() 
{

    my_region k1(2);
    my_region k2(2);

    
    std::cout<<  (k1 == k2) << "\n";
    return 0;
}






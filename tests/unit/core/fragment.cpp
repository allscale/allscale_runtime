

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

using my_region = allscale::region<int> ;
using my_fragment = allscale::fragment<my_region,int>;
//ALLSCALE_REGISTER_FRAGMENT_TYPE(my_region,int);

int main()
{
   my_region test_region(2);
   my_fragment frag(test_region,10);
   
   using value_type = typename my_fragment::value_type;
   value_type val = *(frag.ptr_);
   std::cout <<  "value is "  << val << std::endl;



    return 0;
}


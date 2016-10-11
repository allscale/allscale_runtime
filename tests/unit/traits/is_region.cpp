
#include <iostream>
#include <array>
#include <string>
#include <type_traits>
#include <allscale/region.hpp>
//#include <allscale/traits/region_traits.hpp>
#include <typeinfo>


template<typename T>
bool check(T first)
{
    if(allscale::traits::is_region<T>::value){
        return true;
    }
    else
    {
        return false;
    }
}

int main() {
    
    
    // TEST 1: positive check for region
    //typedef allscale::region<int> int_region;
   
   // int_region b;  
    
    
    /*
    if(check(b)){
        std::cout<<"Test 1 PASSED: is_region gives TRUE if called with a region" << std::endl; 
    }
    else{
        std::cout<<"Test 1 FAILED: argument is a region but is_region gives FALSE" << std::endl; 
    }
    //TEST 2: negative test
    std::string k("mulligan");
    if(!check(k)){
        std::cout<<"Test 2 PASSED: is_region gives FALSE if called with a std::string" << std::endl; 
    }
    else{
        std::cout<<"Test 2 FAILED: argument is not region but is_region gives TRUE" << std::endl; 
    }
*/

    return 0;
}







#include <iostream>
#include <array>
#include <string>
#include <type_traits>
#include <allscale/data_item.hpp>
#include <allscale/traits/data_item_traits.hpp>
#include <typeinfo>

/*
template <typename T>
typename std::enable_if<
    allscale::traits::is_treeture<T>::value,
    typename allscale::traits::treeture_traits<T>::fuck_type>::type
blubber(T first)
{


}


*/



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


int main() {
    
    
    // TEST 1: positive check for treeture
    //typedef allscale::data_item<int> test_item;
    //test_item b; 
  /*
    if(check(b)){
        std::cout<<"Test 1 PASSED: is_data_item gives TRUE if called with a data_item" << std::endl; 
    }
    else{
        std::cout<<"Test 1 FAILED: argument is a data_item but is_data_item gives FALSE" << std::endl; 
    }
   
    //TEST 2: negative test
    std::string k("mulligan");
    if(!check(k)){
        std::cout<<"Test 2 PASSED: is_data_item gives FALSE if called with a std::string" << std::endl; 
    }
    else{
        std::cout<<"Test 2 FAILED: argument is not data_item but is_data_item gives TRUE" << std::endl; 
    }
*/


    return 0;
}






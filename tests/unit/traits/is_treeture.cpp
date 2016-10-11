
#include <iostream>
#include <array>
#include <string>
#include <type_traits>
#include <allscale/treeture.hpp>
#include <allscale/traits/treeture_traits.hpp>
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
    if(allscale::traits::is_treeture<T>::value){
        //std::cout<<"allscale::traits::is_treeture<T>::value==TRUE"<<std::endl;
        return true;
    }
    else
    {
        //std::cout<<"allscale::traits::is_treeture<T>::value==FALSE"<<std::endl;
        return false;
    }
}


void should_give_true(){
    typedef allscale::treeture<int> int_treeture;
    int_treeture b; 
}

int main() {
    
    
    // TEST 1: positive check for treeture
    typedef allscale::treeture<int> int_treeture;
    int_treeture b; 
    if(check(b)){
        std::cout<<"Test 1 PASSED: is_treeture gives TRUE if called with a treeture" << std::endl; 
    }
    else{
        std::cout<<"Test 1 FAILED: argument is a treeture but is_treeture gives FALSE" << std::endl; 
    }
   
    //TEST 2: negative test
    std::string k("mulligan");
    if(!check(k)){
        std::cout<<"Test 2 PASSED: is_treeture gives FALSE if called with a std::string" << std::endl; 
    }
    else{
        std::cout<<"Test 2 FAILED: argument is not treeture but is_treeture gives TRUE" << std::endl; 
    }



    return 0;
}






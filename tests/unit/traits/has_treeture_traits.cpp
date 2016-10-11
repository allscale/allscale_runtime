
#include <iostream>
#include <array>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <allscale/traits/is_treeture.hpp>
#include <allscale/treeture.hpp>
#include <allscale/traits/treeture_traits.hpp>


#include <hpx/dataflow.hpp>
#include <hpx/include/serialization.hpp>

bool check_point = false;

template<typename F>
typename std::enable_if<
    allscale::traits::is_treeture<F>::value,
    typename allscale::traits::treeture_traits<F>::future_type
    >::type
do_shit(F & f){
    
    typedef typename allscale::traits::treeture_traits<F>::future_type my_type;
    my_type k;
    check_point = true;
    return k;
}


template<typename F>
typename std::enable_if<
    !allscale::traits::is_treeture<F>::value,
    int
    >::type
do_shit(F & f){
    check_point = false;
    return 0;
}




template<typename T>
bool check(T first)
{
    
    //typedef typename allscale::traits::treeture_traits<T>::future_type my_type;
    
    do_shit(first);
   // std::cout << typeid(k).name() << std::endl;
    return check_point;
    /*    
    if(allscale::traits::is_treeture<T>::value){
        //std::cout<<"allscale::traits::is_treeture<T>::value==TRUE"<<std::endl;
        return true;
    }
    else
    {
        //std::cout<<"allscale::traits::is_treeture<T>::value==FALSE"<<std::endl;
        return false;
    }*/
}

int main() {
    
    
    // TEST 1: positive check for treeture
    typedef allscale::treeture<int> int_treeture;
    int_treeture b; 
    check(b); 
    

   

    if(check(b)){
        std::cout<<"Test 1 PASSED: treeture_traits gives TRUE if called with a treeture" << std::endl; 
    }
    else{
        std::cout<<"Test 1 FAILED: argument is a treeture but treeture_traits gives FALSE" << std::endl; 
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






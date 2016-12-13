#ifndef ALLSCALE_REGION_HPP
#define ALLSCALE_REGION_HPP

#include<allscale/traits/is_region.hpp>
#include <iostream>

#include <hpx/include/serialization.hpp>

namespace allscale {

    
    template <typename T>
    struct region 
    {
        region()
        {
            T tmp;
            region_ = tmp;
        }
        
        region(T r) : region_(r) {}


        bool operator==(region const & r1){
            if(this->region_ == r1.region_){
                return true;
            }
            else{
                return false;
            }
        }

        region merge(region const & region1, region const & region2){
            //TODO fill with useful stuff
            return region1; 
        }

        region intersection(region const & region1, region const & region2){
            //TODO fill with useful stuff
            return region1; 
        }
   
        bool empty(region const & region1){
         
            //TODO fill with useful stuff
         return false;
        }
        
        template <typename Archive>
        void load(Archive & ar,unsigned)
        {
            //ar & impl_;
           // HPX_ASSERT(impl_->valid());

        }


        std::string to_string()
        {
            return "to string method called";
        }

        template <typename Archive>
        void serialize(Archive & ar, unsigned)
        {
            ar & region_;
        }

        T region_;
        
    
    };

 


    namespace traits
    {
        template <typename T>
        struct is_region< ::allscale::region<T>>
        : std::true_type
        {};

 
    }



}




















































#endif

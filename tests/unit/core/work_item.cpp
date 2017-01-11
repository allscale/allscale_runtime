#include <allscale/no_split.hpp>
#include <allscale/no_serialization.hpp>
#include <allscale/work_item.hpp>
#include <allscale/treeture.hpp>
#include <allscale/work_item_description.hpp>

#include <allscale/data_item.hpp>
#include <allscale/treeture.hpp>
#include <allscale/data_item_description.hpp>
#include <allscale/region.hpp>
#include <allscale/fragment.hpp>


#include <unistd.h>
#include <iostream>
//#include <allscale/scheduler.hpp>
#include <hpx/hpx_main.hpp>
#include <hpx/include/unordered_map.hpp>


typedef std::int64_t int_type;

typedef allscale::region<int> my_region;
using my_fragment = allscale::fragment<my_region,int>;
typedef allscale::data_item_description<my_region,my_fragment,int> descr;

typedef allscale::data_item<descr> test_data_item;

ALLSCALE_REGISTER_DATA_ITEM_TYPE(descr);
ALLSCALE_REGISTER_TREETURE_TYPE(int_type);



struct A{

    A()
    {
        std::cout<<" default ctor of A" << std::endl;
    }

    A(A const& other ){
        std::cout << " copy ctor of A" << std::endl;
    }


    A(A&& other ){
        std::cout << "  move  ctor of A" << std::endl;
    }


    A& operator=(const A& other)
    {
        std::cout << " copy assgn operator of A " << std::endl;
        k = other.k;
        return *this;
    }


    std::string k = "dwwaddwa";
    
};
struct simple_variant
{
    static constexpr bool valid = true;
    using result_type = std::int64_t;
    template <typename Closure>
    static allscale::treeture<std::int64_t> execute(Closure const& closure)
    {
//        A a_one = hpx::util::get<2>(closure);
         test_data_item td_one = hpx::util::get<2>(closure);
        //auto klapper = hpx::util::get<0>(closure); 
        //using value_type = typename my_fragment::value_type;
        //value_type val = *(td_one.fragment_.ptr_);
      //  std::cout<< "value of first data item is " << val << std::endl;
        
        return

            allscale::treeture<std::int64_t>{ hpx::util::get<0>(closure) + 
                                                    hpx::util::get<1>(closure)
            };
    }
};

struct simple_name
{
    static const char *name()
    {
        return "simple";
    }
};


using simple_work_item_descr = 
    allscale::work_item_description<
        std::int64_t,
        simple_name,
        allscale::no_serialization,
        allscale::no_split<std::int64_t>,
        simple_variant
    >;


/*
int main(int argc, char** argv)
{
    // set parent id
    static allscale::this_work_item::id main_id(0);
    allscale::this_work_item::set_id(main_id);
    
    using result_type = simple_work_item_descr::result_type;
    allscale::treeture<result_type> trs(hpx::find_here());
    auto test_item = allscale::work_item(simple_work_item_descr(),trs,2,8);
    hpx::apply(&allscale::work_item::process, test_item);
    HPX_ASSERT(trs.valid());
    auto res = trs.get_result();
    std::cout << "result is: " << res << std::endl;
    
    return 0;
}

*/





int hpx_main(int argc, char* argv[])
{

    descr test_descr;
   

    my_region test_region(2);
    my_fragment frag(test_region,15);
    test_data_item td(hpx::find_here(),test_descr,frag);


    my_region test_region2(3);
    my_fragment frag2(test_region2,20);
    test_data_item td2(hpx::find_here(),test_descr,frag2);
 
    //A a;
    /*
    test_data_item td3 = td2; 
    
    using value_type = typename my_fragment::value_type;
    value_type val = *(td3.fragment_.ptr_);
    std::cout<< "value of first data item is " << val << std::endl;
*/
    
    
    static allscale::this_work_item::id main_id(0);
    allscale::this_work_item::set_id(main_id);

    using result_type = simple_work_item_descr::result_type;
    allscale::treeture<result_type> trs(hpx::find_here());
    auto test_item = allscale::work_item(simple_work_item_descr(),trs,2,4,td);
    hpx::apply(&allscale::work_item::process, test_item);
    HPX_ASSERT(trs.valid());
    auto res = trs.get_result();
    std::cout << "result is: " << res << std::endl;
  



    return hpx::finalize();
}


int main(int argc, char* argv[])
{
    return hpx::init(argc, argv);
}


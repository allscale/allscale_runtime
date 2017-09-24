#include <allscale/api/user/data/scalar.h>
#include <allscale/serialization_wrapper.hpp>

#include <hpx/hpx.hpp>
#include <hpx/hpx_main.hpp>

template <typename T>
T test(T t)
{
    return t;
}

template <typename T>
struct test_action
  : hpx::actions::make_action<decltype(&test<T>), &test<T>, test_action<T>>
{};

template <typename T>
void do_test()
{
    test_action<T> action;

    T t;
    action(hpx::find_here(), std::move(t));
}

int hpx_main(int argc, char **argv)
{
    do_test<allscale::api::user::data::Scalar<int>::region_type>();
    do_test<allscale::api::user::data::Scalar<int>::fragment_type>();
    return hpx::finalize();
}

int main(int argc, char **argv)
{
    return hpx::init(argc, argv);
}

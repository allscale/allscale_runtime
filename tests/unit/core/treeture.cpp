
#include <allscale/treeture.hpp>

#include <hpx/hpx_main.hpp>

#include <iostream>
#include <hpx/util/lightweight_test.hpp>
#include <hpx/runtime/actions/plain_action.hpp>

ALLSCALE_REGISTER_TREETURE_TYPE_(int, allscale_int);

void test1()
{
    allscale::treeture<void> null_parent;
    {
        allscale::treeture<int> t(allscale::parent_arg{}, null_parent);
        hpx::future<int> f = t.get_future();
        t.set_value(9);
        HPX_TEST_EQ(f.get(), 9);
    }
    hpx::future<int> f;
    {
        allscale::treeture<int> t(allscale::parent_arg{}, null_parent);
        f = t.get_future();
        t.set_value(9);
    }
    HPX_TEST_EQ(f.get(), 9);
}

void remote_get(allscale::treeture<int> t)
{
    allscale::treeture<void> tt(t);
    HPX_TEST_EQ(t.get_future().get(), 4711);
    tt.get_future().get();
}
HPX_PLAIN_ACTION(remote_get);

void remote_set(allscale::treeture<int> t)
{
    t.set_value(1377);
}
HPX_PLAIN_ACTION(remote_set);

void test2()
{
    allscale::treeture<void> null_parent;
    auto localities = hpx::find_all_localities();
    {
        remote_get_action act;
        for (auto id: localities)
        {
            allscale::treeture<int> t(allscale::parent_arg{}, null_parent);
            auto f = hpx::async(act, id, t);
            t.set_value(4711);
            f.get();
        }
    }
    {
        remote_get_action act;
        for (auto id: localities)
        {
            hpx::future<void> f;
            {
                allscale::treeture<int> t(allscale::parent_arg{}, null_parent);
                f = hpx::async(act, id, t);
                t.set_value(4711);
            }
            f.get();
        }
    }
    {
        remote_set_action act;
        for (auto id: localities)
        {
            allscale::treeture<int> t(allscale::parent_arg{}, null_parent);
            hpx::apply(act, id, t);
            HPX_TEST_EQ(t.get_future().get(), 1377);
        }
    }
    {
        remote_set_action act;
        for (auto id: localities)
        {
            hpx::future<int> f;
            {
                allscale::treeture<int> t(allscale::parent_arg{}, null_parent);
                f = t.get_future();
                hpx::apply(act, id, t);
            }
            HPX_TEST_EQ(f.get(), 1377);
        }
    }
}

void test3()
{
    allscale::treeture<void> null_parent;
    {
        allscale::treeture<int> t(allscale::parent_arg{}, null_parent);
        allscale::treeture<void> tt(t);
        hpx::future<int> f = t.get_future();
        t.set_value(9);
        HPX_TEST_EQ(f.get(), 9);
        HPX_TEST(tt.get_future().is_ready());
    }
}

int main()
{
    test1();
    test2();
    test3();

    return hpx::util::report_errors();
}

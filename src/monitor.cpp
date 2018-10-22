#include <allscale/monitor.hpp>
#include <allscale/work_item.hpp>
#include <allscale/components/monitor.hpp>

#include <hpx/include/components.hpp>
#include <hpx/util/detail/yield_k.hpp>
#include <vector>

//HPX_REGISTER_COMPONENT_MODULE()

typedef hpx::components::component<allscale::components::monitor> monitor_component;
HPX_REGISTER_COMPONENT(monitor_component)

namespace allscale {
    components::monitor* monitor::run(std::size_t rank)
    {
        return get_ptr();
    }

    void monitor::stop()
    {
        get().stop();
//         get_ptr().reset();
    }


    std::vector<monitor::event_function>& event_functions(monitor::event e)
    {
        static std::array<std::vector<monitor::event_function>, monitor::last_> functions;

        return functions[e];
    }

    void monitor::connect(event e, event_function f)
    {
        event_functions(e).push_back(f);
    }


    void monitor::disconnect(event e, event_function f)
    {
//        event_functions(e).erase(std::remove(event_functions(e).begin(),
//	     event_functions(e).end(), f), event_functions(e).end());
    }


    void monitor::signal(event e, work_item const& w)
    {
        for(auto& f: event_functions(e))
        {
            f(w);
        }
    }

    components::monitor &monitor::get()
    {
        HPX_ASSERT(get_ptr());
        return *get_ptr();
    }

    components::monitor *monitor::get_ptr()
    {
        static monitor m(hpx::get_locality_id());
        components::monitor* res = m.component_.get();
        for (std::size_t k = 0; !res; ++k)
        {
            hpx::util::detail::yield_k(k, "get component...");
            res = m.component_.get();
        }
        res->init();
        return res;
    }

    monitor::monitor(std::size_t rank)
    {
        hpx::id_type gid =
            hpx::new_<components::monitor>(hpx::find_here(), rank).get();

        component_ = hpx::get_ptr<components::monitor>(gid).get();
//         component_->init();
//
//         char *env = std::getenv("ALLSCALE_MONITOR");
//         if(env && env[0] == '0')
//         {
//             return;
//         }
//
    }

}

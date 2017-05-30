#include <allscale/monitor.hpp>
#include <allscale/work_item.hpp>
#include <allscale/components/monitor.hpp>

#include <hpx/include/components.hpp>
#include <vector>

//HPX_REGISTER_COMPONENT_MODULE()

typedef hpx::components::component<allscale::components::monitor> monitor_component;
HPX_REGISTER_COMPONENT(monitor_component)

namespace allscale {

    std::size_t monitor::rank_ = std::size_t(-1);

    components::monitor* monitor::run(std::size_t rank)
    {
        rank_ = rank;
        return get_ptr().get();
    }

    void monitor::stop()
    {
        get().stop();
        get_ptr().reset();
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

    void monitor::signal(event e, work_item const& w)
    {
        for(auto& f: event_functions(e))
        {
            f(w);
        }
    }

    components::monitor &monitor::get()
    {
        return *get_ptr();
    }

    std::shared_ptr<components::monitor> &monitor::get_ptr()
    {
        static monitor m(rank_);
        return m.component_;
    }

    monitor::monitor(std::size_t rank)
    {
        hpx::id_type gid =
            hpx::new_<components::monitor>(hpx::find_here(), rank).get();

        hpx::register_with_basename("allscale/monitor", gid, rank).get();

        component_ = hpx::get_ptr<components::monitor>(gid).get();
        component_->init();
    }

}

#include <allscale/monitor.hpp>
#include <allscale/work_item.hpp>

#include <vector>

namespace allscale {
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
}

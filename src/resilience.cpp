#include <allscale/components/resilience.hpp>
#include <allscale/resilience.hpp>

#include <hpx/include/components.hpp>

typedef hpx::components::component<allscale::components::resilience> resilience_component;
HPX_REGISTER_COMPONENT(resilience_component)

namespace allscale {
    std::size_t resilience::rank_ = std::size_t(-1);

    components::resilience* resilience::run(std::size_t rank)
    {
        rank_ = rank;
        return get_ptr().get();
    }

    resilience::resilience(std::size_t rank)
    {
        hpx::id_type gid =
            hpx::new_<components::resilience>(hpx::find_here(), rank).get();

        hpx::register_with_basename("allscale/resilience", gid, rank).get();

        component_ = hpx::get_ptr<components::resilience>(gid).get();
        component_->init();
    }

    std::shared_ptr<components::resilience> &resilience::get_ptr()
    {
        static resilience m(rank_);
        return m.component_;
    }
}

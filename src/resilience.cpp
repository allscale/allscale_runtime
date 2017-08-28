#include <allscale/components/resilience.hpp>
#include <allscale/resilience.hpp>

#include <hpx/include/components.hpp>
#include <hpx/util/detail/yield_k.hpp>

typedef hpx::components::component<allscale::components::resilience> resilience_component;
HPX_REGISTER_COMPONENT(resilience_component)


namespace allscale {
    std::size_t resilience::rank_ = std::size_t(-1);

    components::resilience* resilience::run(std::size_t rank)
    {
        rank_ = rank;
        return get_ptr();
    }

    resilience::resilience(std::size_t rank)
    {
        hpx::id_type gid =
            hpx::new_<components::resilience>(hpx::find_here(), rank).get();

        hpx::register_with_basename("allscale/resilience", gid, rank).get();

        component_ = hpx::get_ptr<components::resilience>(gid).get();
        component_->init();
        component_->failure_detection_loop_async();
    }

    void resilience::stop() {
        get_ptr()->shutdown();
    }

    components::resilience *resilience::get_ptr()
    {
        static resilience m(rank_);
        components::resilience* res = m.component_.get();
        for (std::size_t k = 0; !res; ++k)
        {
            hpx::util::detail::yield_k(k, "get component...");
            res = m.component_.get();
        }
        return res;
    }

    components::resilience &resilience::get()
    {
        HPX_ASSERT(get_ptr());
        return *get_ptr();
    }

    hpx::id_type resilience::get_protectee() {
        return get_ptr()->get_protectee();
    }
}

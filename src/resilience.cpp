#include <allscale/components/resilience.hpp>
#include <allscale/resilience.hpp>

#include <hpx/include/components.hpp>
#include <hpx/util/detail/yield_k.hpp>

#include <csignal>

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
    }

    void resilience::stop() {
        get_ptr()->shutdown(1);
    }

    void resilience::global_w_exec_start_wrapper(const work_item &work) {
        get().w_exec_start_wrapper(work);
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

    std::pair<hpx::id_type,uint64_t> resilience::get_protectee() {
        return get_ptr()->get_protectee();
    }

    bool resilience::rank_running(uint64_t rank) {
        if (rank_ == std::size_t(-1) || rank == rank_)
            return true;

        return get_ptr()->rank_running(rank);
    }

    // ignore signum here ...
    void resilience::handle_my_crash(int signum)
    {
        HPX_ASSERT(get_ptr());
        get_ptr()->handle_my_crash();
    }
}

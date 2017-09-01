
#include <allscale/this_work_item.hpp>
#include <allscale/treeture.hpp>
#include <allscale/work_item.hpp>
#include <allscale/detail/work_item_impl_base.hpp>

#include <hpx/include/threads.hpp>
#include <hpx/runtime/get_os_thread_count.hpp>
#include <hpx/runtime/resource/detail/partitioner.hpp>
#include <hpx/runtime/threads/threadmanager.hpp>
#include <hpx/runtime/threads/topology.hpp>

#include <vector>

namespace allscale { namespace this_work_item {

    void set_id_impl(id const& id_)
    {
        hpx::this_thread::set_thread_data(reinterpret_cast<std::size_t>(&id_));
    }

    id* get_id_impl()
    {
//         if (hpx::this_thread::get_thread_data() == 0)
//         {
//             return nullptr
//         }
        return reinterpret_cast<id*>(hpx::this_thread::get_thread_data());
    }

    id::id()
    {
        config_.rank_ = -1;
        config_.numa_domain_ = -1;
        config_.locality_depth_ = -1;
        config_.numa_depth_ = -1;
        config_.thread_depth_ = -1;
        config_.gpu_depth_ = -1;
        config_.needs_checkpoint_ = false;
    }

    std::size_t get_num_numa_nodes()
    {
        auto const& topo = hpx::threads::get_topology();

        std::size_t numa_nodes = topo.get_number_of_numa_nodes();
        if (numa_nodes == 0)
            numa_nodes = topo.get_number_of_sockets();
        std::vector<hpx::threads::mask_type> node_masks(numa_nodes);

        auto& rp = hpx::resource::get_partitioner();

        std::size_t num_os_threads = hpx::get_os_thread_count();
        for (std::size_t num_thread = 0; num_thread != num_os_threads;
             ++num_thread)
        {
            std::size_t pu_num = rp.get_pu_num(num_thread);
            std::size_t numa_node = topo.get_numa_node_number(pu_num);

            auto const& mask = topo.get_thread_affinity_mask(pu_num);

            std::size_t mask_size = hpx::threads::mask_size(mask);
            for (std::size_t idx = 0; idx != mask_size; ++idx)
            {
                if (hpx::threads::test(mask, idx))
                {
                    hpx::threads::set(node_masks[numa_node], idx);
                }
            }
        }

        // Sort out the masks which don't have any bits set
        std::size_t res = 0;

        for (auto& mask : node_masks)
        {
            if (hpx::threads::any(mask))
            {
                ++res;
            }
        }

        return res;
    }

    std::size_t get_num_numa_cores(std::size_t domain)
    {
        auto const& topo = hpx::threads::get_topology();

        std::size_t numa_nodes = topo.get_number_of_numa_nodes();
        if (numa_nodes == 0)
            numa_nodes = topo.get_number_of_sockets();
        std::vector<hpx::threads::mask_type> node_masks(numa_nodes);

        auto& rp = hpx::resource::get_partitioner();

        std::size_t res = 0;
        std::size_t num_os_threads = hpx::get_os_thread_count();
        for (std::size_t num_thread = 0; num_thread != num_os_threads;
             ++num_thread)
        {
            std::size_t pu_num = rp.get_pu_num(num_thread);
            std::size_t numa_node = topo.get_numa_node_number(pu_num);
            if(numa_node != domain) continue;

            auto const& mask = topo.get_thread_affinity_mask(pu_num);

            std::size_t mask_size = hpx::threads::mask_size(mask);
            for (std::size_t idx = 0; idx != mask_size; ++idx)
            {
                if (hpx::threads::test(mask, idx))
                {
                    ++res;
                }
            }
        }
        return res;
    }

    id::id(std::size_t i)
      : id_(1, i),
        next_id_(0)
    {
        HPX_ASSERT(i == 0);
        config_.rank_ = 0;
        config_.numa_domain_ = 0;
        config_.locality_depth_ = hpx::get_num_localities(hpx::launch::sync);
        config_.numa_depth_ = get_num_numa_nodes();
        config_.thread_depth_ = -1;
        config_.gpu_depth_ = -1;
        config_.needs_checkpoint_ = false;
    }

    void id::setup_left()
    {
        auto& parent = parent_->config_;
        config_.rank_ = parent.rank_;
        config_.numa_domain_ = parent.numa_domain_;
        if (parent.locality_depth_ > 1)
        {
            config_.locality_depth_ = parent.locality_depth_ / 2;
            config_.numa_depth_ = parent.numa_depth_;
            config_.thread_depth_ = parent.thread_depth_;
        }
        else
        {
            config_.locality_depth_ = 1;
            config_.needs_checkpoint_ = parent.locality_depth_ > 1;
            if(parent.numa_depth_ > 1)
            {
                config_.numa_depth_ = parent.numa_depth_ / 2;
                config_.thread_depth_ = parent.thread_depth_;
            }
            else
            {
                config_.numa_depth_ = 1;
                config_.thread_depth_ = parent.thread_depth_ / 2;
            }
        }
    }

    void id::setup_right()
    {
        auto& parent = parent_->config_;
        if (parent.locality_depth_ > 1)
        {
            config_.rank_ = parent.rank_ + (parent.locality_depth_ / 2);
            config_.numa_domain_ = parent.numa_domain_;
            config_.locality_depth_ = (parent.locality_depth_ / 2.0) + 0.5;
            config_.numa_depth_ = parent.numa_depth_;
        }
        else
        {
            config_.rank_ = parent.rank_;
            config_.needs_checkpoint_ = parent.locality_depth_ > 1;
            config_.locality_depth_ = 1;
            if(parent.numa_depth_ > 1)
            {
                config_.numa_domain_ = parent.numa_domain_ + (parent.numa_depth_ / 2);
                config_.numa_depth_ = (parent.numa_depth_ / 2.0) + 0.5;
                config_.thread_depth_ = parent.thread_depth_;
            }
            else
            {
                config_.numa_domain_ = parent.numa_domain_;
                config_.numa_depth_ = 1;
                config_.thread_depth_ = (parent.thread_depth_ / 2.0) + 0.5;
            }
        }
    }

    bool id::needs_checkpoint() const
    {
        return config_.needs_checkpoint_;
    }

    bool id::splittable() const
    {
        if (config_.locality_depth_ == 1 && config_.numa_depth_ == 1)
        {
            HPX_ASSERT(config_.thread_depth_ != std::uint32_t(-1));
            return config_.thread_depth_ != 1;
        }
        return config_.locality_depth_ != 1 || config_.numa_depth_ != 1;
    }

    std::size_t id::rank() const
    {
        HPX_ASSERT(config_.rank_ != std::uint64_t(-1));
        return config_.rank_;
    }

    std::size_t id::numa_domain() const
    {
        return config_.numa_domain_;
    }


//     void id::set(std::shared_ptr<detail::work_item_impl_base> wi)
    void id::set(detail::work_item_impl_base* wi, bool is_first)
    {
        id& parent = get_id();
        // set the parent with a null deleter, we only need to delete the pointer
        // if we serialized the id...
        parent_.reset(&parent, [](void*){});
        next_id_ = 0;
        id_ = parent.id_;
        id_.push_back(parent.next_id_++);


        HPX_ASSERT(parent.config_.rank_ != std::uint64_t(-1));
        HPX_ASSERT(parent.config_.locality_depth_ != std::uint64_t(-1));
        HPX_ASSERT(parent.config_.numa_depth_ != std::uint8_t(-1));
//         HPX_ASSERT(parent.thread_depth_ != std::size_t(-1));

        if (is_first)
        {
            config_.rank_ = parent.config_.rank_;
            config_.numa_domain_ = parent.config_.numa_domain_;
            config_.locality_depth_ = parent.config_.locality_depth_;
            config_.numa_depth_ = parent.config_.numa_depth_;
            config_.gpu_depth_ = parent.config_.gpu_depth_;
        }
        else
        {
            if (id_.back() == 0)
            {
                setup_left();
            }
            else
            {
                setup_right();
            }

        }

        // Once we reached out to the locality leaf, we need to set the
        // number of threads on the given NUMA node to create enough
        // parallelism. There is currently a oversubscription factor of
        // 1.5 in the number of threads per NUMA domain.
        if (config_.locality_depth_ == 1 && config_.numa_depth_ == 1 && config_.thread_depth_ == std::uint32_t(-1))
        {
            // We round up here...
            std::size_t num_cores = get_num_numa_cores(config_.numa_domain_);
            if (num_cores == 1)
                config_.thread_depth_ = 1;
            else
                config_.thread_depth_ = std::pow(num_cores, 1.5) + 0.5;
        }

//         wi_ = std::move(wi);
//         wi_ = wi;
        tres_ = wi->get_treeture();
    }

    id *get_id_ptr()
    {
//         HPX_ASSERT(get_id_impl());
        return get_id_impl();
    }

    id& get_id()
    {
//         HPX_ASSERT(get_id_impl());
        return *get_id_impl();
    }

    void set_id(id const& id_)
    {
        set_id_impl(id_);
    }

    std::string id::name() const
    {
        std::string result;
        for (std::size_t id: id_)
        {
            result += std::to_string(id) + ".";
        }
        result = result.substr(0, result.size()-1);
        return result;
    }

    std::size_t id::last() const
    {
        return id_.back();
    }

    std::size_t id::depth() const
    {
        return id_.size();
    }

    std::size_t id::hash() const
    {
        return std::hash<std::string>()(name());
    }

    id const& id::parent() const
    {
        HPX_ASSERT(parent_);
        return *parent_;
    }

    treeture<void> id::get_treeture() const
    {
        return tres_;
//         return wi_->get_treeture();
    }

    bool operator==(id const& lhs, id const& rhs)
    {
        return lhs.name() == rhs.name();
    }

    bool operator!=(id const& lhs, id const& rhs)
    {
        return lhs.name() != rhs.name();
    }

    bool operator<=(id const& lhs, id const& rhs)
    {
        return lhs.name() <= rhs.name();
    }

    bool operator>=(id const& lhs, id const& rhs)
    {
        return lhs.name() >= rhs.name();
    }

    bool operator<(id const& lhs, id const& rhs)
    {
        return lhs.name() < rhs.name();
    }

    bool operator>(id const& lhs, id const& rhs)
    {
        return lhs.name() > rhs.name();
    }
}}

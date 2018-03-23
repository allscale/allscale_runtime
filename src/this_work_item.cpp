
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


    id::id(std::size_t i)
      : id_(1, i),
        next_id_(0)
    {
        HPX_ASSERT(i == 0);
        config_.rank_ = -1;
        config_.numa_domain_ = 0;
        config_.locality_depth_ = -1;//hpx::get_num_localities(hpx::launch::sync);
        config_.numa_depth_ = -1;//get_num_numa_nodes();
        config_.thread_depth_ = -1;
        config_.gpu_depth_ = -1;
        config_.needs_checkpoint_ = false;
    }

    bool id::split_locality_depth(machine_config const& mconfig)
    {
        auto& parent = parent_->config_;
        if (parent.locality_depth_ == std::uint64_t(-1))
        {
            config_.rank_ = hpx::get_locality_id();
            config_.locality_depth_ = mconfig.num_localities;
            config_.numa_domain_ = parent.numa_domain_;
            return false;
        }
        return true;
    }

    bool id::split_numa_depth(machine_config const& mconfig)
    {
        auto& parent = parent_->config_;
        if (parent.numa_depth_ == std::uint8_t(-1))
        {
            config_.numa_domain_ = 0;
            config_.numa_depth_ = mconfig.thread_depths.size();
            config_.thread_depth_ = -1;
            return false;
        }
        return true;
    }

    // Once we reached out to the locality leaf, we need to set the
    // number of threads on the given NUMA node to create enough
    // parallelism. There is currently a oversubscription factor of
    // 1.5 in the number of threads per NUMA domain.
    bool id::split_thread_depth(machine_config const& mconfig)
    {
        auto& parent = parent_->config_;
        if (parent.thread_depth_ == std::uint32_t(-1))
        {
            config_.thread_depth_ = mconfig.thread_depths[config_.numa_domain_];
            return false;
        }
        return true;
    }

    void id::setup_left(machine_config const& mconfig)
    {
        auto& parent = parent_->config_;
        config_.rank_ = parent.rank_;
        config_.numa_domain_ = parent.numa_domain_;
        if (!split_locality_depth(mconfig)) return;

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

            if(split_numa_depth(mconfig))
            {
                if(parent.numa_depth_ > 1)
                {
                    config_.numa_depth_ = parent.numa_depth_ / 2;
                    config_.thread_depth_ = parent.thread_depth_;
                }
                else
                {
                    config_.numa_depth_ = 1;
                    if (split_thread_depth(mconfig))
                    {
                        config_.thread_depth_ = parent.thread_depth_ / 2;
                    }
                }
            }
        }
    }

    void id::setup_right(machine_config const& mconfig)
    {
        auto& parent = parent_->config_;
        if (!split_locality_depth(mconfig)) return;

        if (parent.locality_depth_ > 1)
        {
            config_.rank_ = parent.rank_ + (parent.locality_depth_ / 2);
            config_.numa_domain_ = parent.numa_domain_;
            config_.locality_depth_ = std::lround(parent.locality_depth_ / 2.0);
            config_.numa_depth_ = parent.numa_depth_;
        }
        else
        {
            config_.rank_ = parent.rank_;
            config_.needs_checkpoint_ = parent.locality_depth_ > 1;
            config_.locality_depth_ = 1;
            if(split_numa_depth(mconfig))
            {
                if(parent.numa_depth_ > 1)
                {
                    config_.numa_domain_ = parent.numa_domain_ + (parent.numa_depth_ / 2);
                    config_.numa_depth_ = std::lround(parent.numa_depth_ / 2.0);
                    config_.thread_depth_ = parent.thread_depth_;
                }
                else
                {
                    config_.numa_domain_ = parent.numa_domain_;
                    config_.numa_depth_ = 1;
                    if (split_thread_depth(mconfig))
                    {
                        config_.thread_depth_ = std::lround(parent.thread_depth_ / 2.0);
                    }
                }
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
            return config_.thread_depth_ > 1;
        }
        return config_.locality_depth_ != 1 || config_.numa_depth_ != 1;
    }

    void id::update_rank(std::size_t rank)
    {
        HPX_ASSERT(config_.rank_ != std::uint64_t(-1));
        config_.rank_ = rank;
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

    std::size_t id::thread_depth() const
    {
        return config_.thread_depth_;
    }

//     void id::set(std::shared_ptr<detail::work_item_impl_base> wi)
    void id::set(detail::work_item_impl_base* wi, machine_config const& mconfig)
    {
        id& parent = get_id();
        // set the parent with a null deleter, we only need to delete the pointer
        // if we serialized the id...
        parent_.reset(&parent, [](void*){});
        next_id_ = 0;
        id_ = parent.id_;
        id_.push_back(parent.next_id_++);

//         HPX_ASSERT(parent.config_.rank_ != std::uint64_t(-1));
//         HPX_ASSERT(parent.config_.locality_depth_ != std::uint64_t(-1));
//         HPX_ASSERT(parent.config_.numa_depth_ != std::uint8_t(-1));
//         HPX_ASSERT(parent.thread_depth_ != std::size_t(-1));

        if (parent.config_.thread_depth_ == 1)
        {
            config_ = parent.config_;
        }
        else
        {
            if (id_.back() == 0)
            {
                setup_left(mconfig);
            }
            else
            {
                setup_right(mconfig);
            }
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

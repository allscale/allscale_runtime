
#include <allscale/this_work_item.hpp>
#include <allscale/treeture.hpp>
#include <allscale/work_item.hpp>

#include <hpx/include/threads.hpp>

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
//       : tres_(make_ready_treeture())
    {}

    void id::set(id const& parent, treeture<void> const& tres)
    {
        next_id_ = 0;
        id_ = parent.id_;
        id_.push_back(get_id().next_id_++);
        tres_ = tres;
    }

    id& get_id()
    {
        HPX_ASSERT(get_id_impl());
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

    id id::parent() const
    {
        id res(*this);

        res.next_id_ = 0;
        res.id_.pop_back();

        return res;
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

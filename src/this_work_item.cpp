
#include <allscale/this_work_item.hpp>

#include <hpx/include/threads.hpp>

namespace allscale { namespace this_work_item {

    id* get_id_impl()
    {
        return reinterpret_cast<id*>(hpx::this_thread::get_thread_data());
    }

    void set_id_impl(id const& id_)
    {
        hpx::this_thread::set_thread_data(reinterpret_cast<std::size_t>(&id_));
    }

    id::id()
      : id_(get_id().id_)
      , next_id_(0)
    {
        id_.push_back(get_id().next_id_++);
    }

    id& get_id()
    {
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

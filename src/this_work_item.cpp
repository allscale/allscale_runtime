
#include <allscale/this_work_item.hpp>


namespace allscale { namespace this_work_item {

    id** get_id_impl()
    {
        static thread_local id* id_ = nullptr;
        return &id_;
    }

    id::id()
      : id_(get_id().id_)
      , next_id_(0)
    {
        id_.push_back((*get_id_impl())->next_id_++);
    }

    id& get_id()
    {
        static id main_id(0);
        if (*get_id_impl() == nullptr)
        {
            set_id(main_id);
        }
        return **get_id_impl();
    }

    void set_id(id& id)
    {
        *get_id_impl() = &id;
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

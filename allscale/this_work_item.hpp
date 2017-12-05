
#ifndef ALLSCALE_THIS_WORK_ITEM_HPP
#define ALLSCALE_THIS_WORK_ITEM_HPP

#include <allscale/treeture.hpp>

#include <cstddef>
#include <list>
#include <string>

#include <memory>

#include <hpx/include/serialization.hpp>
#include <hpx/runtime/serialization/list.hpp>

namespace allscale {
    namespace detail {
        struct work_item_impl_base;
    }
    struct work_item;
}

namespace allscale {

    struct machine_config
    {
        std::size_t num_localities;
        std::vector<std::size_t> thread_depths;
    };

    namespace this_work_item {
    struct id;

    id& get_id();
    id* get_id_ptr();
    void set_id(id const& id);

    struct id
    {

        id(std::size_t i);

        id();

        void set(detail::work_item_impl_base*, machine_config const& mconfig);

        std::string name() const;
        std::size_t last() const;
        std::size_t depth() const;

        std::size_t hash() const;

        id const& parent() const;

        bool needs_checkpoint() const;
        bool splittable() const;
        std::size_t rank() const;
        std::size_t numa_domain() const;

        treeture<void> get_treeture() const;

        explicit operator bool() const
        {
            return !id_.empty();
        }

    private:
        friend bool operator==(id const& lhs, id const& rhs);
        friend bool operator!=(id const& lhs, id const& rhs);
        friend bool operator<=(id const& lhs, id const& rhs);
        friend bool operator>=(id const& lhs, id const& rhs);
        friend bool operator<(id const& lhs, id const& rhs);
        friend bool operator>(id const& lhs, id const& rhs);

        struct tree_config
        {
            std::uint64_t rank_;
            std::uint64_t locality_depth_;
            std::uint32_t thread_depth_;
            std::uint8_t numa_domain_;
            std::uint8_t numa_depth_;
            std::uint8_t gpu_depth_;
            bool needs_checkpoint_;

            template <typename Archive>
            void serialize(Archive& ar, unsigned)
            {
                ar & rank_;
                ar & numa_domain_;
                ar & locality_depth_;
                ar & numa_depth_;
                ar & thread_depth_;
                ar & gpu_depth_;
                ar & needs_checkpoint_;
            }
        };
        std::shared_ptr<id> parent_;
        tree_config config_;

        bool split_numa_depth(machine_config const&);
        bool split_thread_depth(machine_config const&);
        void setup_left(machine_config const&);
        void setup_right(machine_config const&);

        std::list<std::size_t> id_;
        std::size_t next_id_;
        treeture<void> tres_;
//         detail::work_item_impl_base* wi_;
//         std::shared_ptr<detail::work_item_impl_base> wi_;

        friend class hpx::serialization::access;
        template <typename Archive>
        void serialize(Archive& ar, unsigned)
        {
            ar & parent_;
            ar & config_;
            ar & id_;
            ar & next_id_;
            ar & tres_;
        }
    };
}}

namespace std
{
    template <>
    struct hash<allscale::this_work_item::id>
    {
        std::size_t operator()(allscale::this_work_item::id const& id) const
        {
            return id.hash();
        }
    };
}

HPX_IS_BITWISE_SERIALIZABLE(allscale::this_work_item::id::tree_config);

#endif

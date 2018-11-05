#ifndef ALLSCALE_DATA_ITEM_REFERENCE_HPP
#define ALLSCALE_DATA_ITEM_REFERENCE_HPP

#include <hpx/util/atomic_count.hpp>
#include <hpx/runtime/get_locality_id.hpp>
#include <hpx/traits/is_bitwise_serializable.hpp>

#include <atomic>
#include <cstdint>
#include <iostream>

///////////////////////////////////////////////////////////////////////////

namespace allscale {
    struct data_item_id
    {
        std::uint32_t locality_;
        std::uint32_t id_;

        friend bool operator==(data_item_id const& lhs, data_item_id const& rhs)
        {
            return lhs.locality_ == rhs.locality_ && lhs.id_ == rhs.id_;
        }

        friend bool operator!=(data_item_id const& lhs, data_item_id const& rhs)
        {
            return !(lhs == rhs);
        }

        friend std::ostream& operator<<(std::ostream& os, data_item_id const& id)
        {
            os << "D-" << id.locality_ << '.' << id.id_;
            return os;
        }

        template <typename Archive>
        void serialize(Archive& ar, unsigned)
        {
            ar & locality_;
            ar & id_;
        }

        static data_item_id create()
        {
            static hpx::util::atomic_count id(0);
            return {hpx::get_locality_id(), static_cast<std::uint32_t>(++id - 1)};
        }
    };

    template<typename DataItemType>
    class data_item_reference
    {
    public:
        typedef DataItemType data_item_type;
        typedef typename DataItemType::shared_data_type shared_data_type;
        typedef typename DataItemType::fragment_type fragment_type;

        // NOTE: data_item_reference currently implements a single ownership
        // semantic as follows:
        // Only the first data_item_reference that was created has ownership
        // Ownership is transferred with move and copy construction or assignment
        // Serialization does not propagate the ownership.
        //
        // CAUTION: This requires the user to ensure that the owning
        // data_item_reference stays alive long enough!
        //
        // This is implemented by using the (global) reference counting mechanism
        // of HPX components.
        // If true global reference counting is required in the end, the
        // make_unmanaged call in the save function needs to be removed.

        data_item_reference() : cache(nullptr)
        {}

        explicit data_item_reference(data_item_id id) : id_(id), cache(nullptr)
        {
        }

        data_item_reference(data_item_reference const& other)
          : id_(other.id_)
          , cache(nullptr)
        {}

        data_item_reference(data_item_reference&& other) noexcept
          : id_(std::move(other.id_))
          , cache(nullptr)
        {
            other.cache.store(nullptr);
        }

        data_item_reference& operator=(data_item_reference const& other)
        {
            id_ = other.id_;
//             cache.store(other.cache.load(std::memory_order_acquire), std::memory_order_release);

            return *this;
        }

        data_item_reference& operator=(data_item_reference&& other) noexcept
        {
            id_ = other.id_;
//             cache.store(other.cache.load(std::memory_order_acquire), std::memory_order_release);
//             other.cache.store(nullptr, std::memory_order_release);

            return *this;
        }

        template <typename Archive>
        void serialize(Archive & ar, unsigned) const
        {
            ar & id_;
        }

        data_item_id const& id() const
        {
            return id_;
        }

        fragment_type* getFragmentHint() const {
            return cache.load(std::memory_order_acquire);
        }

        fragment_type* setFragmentHint(fragment_type* hint) const {
            cache.exchange(hint, std::memory_order_acq_rel);
            return hint;
        }

    private:
        data_item_id id_;

        // a transient cache for the referenced fragment
        mutable std::atomic<fragment_type*> cache;

    };
}

namespace std
{
    template <>
    struct hash<allscale::data_item_id>
    {
        std::size_t operator()(allscale::data_item_id const& id) const
        {
            return std::hash<std::size_t>()(id.id_);
        }
    };
}

// HPX_IS_BITWISE_SERIALIZABLE(allscale::data_item_id);

#endif

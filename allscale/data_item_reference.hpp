#ifndef ALLSCALE_DATA_ITEM_REFERENCE_HPP
#define ALLSCALE_DATA_ITEM_REFERENCE_HPP

#include <hpx/runtime/components/server/component_base.hpp>
#include <hpx/runtime/naming/id_type.hpp>
#include <hpx/lcos/future.hpp>
#include <hpx/include/serialization.hpp>

///////////////////////////////////////////////////////////////////////////

namespace allscale {
    namespace detail
    {
        struct id_holder
          : hpx::components::component_base<id_holder>
        {
            typedef void(*release_function_type)(hpx::naming::gid_type const&);

            explicit id_holder(release_function_type release_function)
              : release_function_(release_function)
            {}

            ~id_holder()
            {
                HPX_ASSERT(release_function_);
                HPX_ASSERT(this->get_id());
                std::cout << "Release DI " << this->get_id() << "\n";
                release_function_(this->get_id().get_gid());
            }

            release_function_type release_function_;
        };
    }

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

        explicit data_item_reference(hpx::future<hpx::id_type> fid) : cache(nullptr)
        {
            hpx::id_type id(fid.get());
            id.make_unmanaged();
            id_ = id.get_gid();
        }

        explicit data_item_reference(hpx::naming::gid_type id) : id_(id), cache(nullptr)
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

        hpx::naming::gid_type const& id() const
        {
            return id_;
        }

        fragment_type* getFragmentHint() const {
            return nullptr;
//             return cache.load(std::memory_order_acquire);
        }

        fragment_type* setFragmentHint(fragment_type* hint) const {
            return nullptr;
//             cache.exchange(hint, std::memory_order_acq_rel);
//             return hint;
        }

    private:
        hpx::naming::gid_type id_;

        // a transient cache for the referenced fragment
        mutable std::atomic<fragment_type*> cache;

    };
}

#endif

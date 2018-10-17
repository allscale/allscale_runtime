#ifndef ALLSCALE_DATA_ITEM_TASK_REQUIREMENTS_HPP
#define ALLSCALE_DATA_ITEM_TASK_REQUIREMENTS_HPP

#include <allscale/hierarchy.hpp>
#include <allscale/data_item_manager/add_allowance.hpp>
#include <allscale/data_item_manager/check_write_requirements.hpp>
#include <allscale/data_item_manager/get_missing_regions.hpp>
#include <allscale/data_item_manager/acquire.hpp>
#include <allscale/data_item_manager/show.hpp>

#include <hpx/include/serialization.hpp>

#include <hpx/lcos/future.hpp>
#include <hpx/util/assert.hpp>
#include <hpx/util/debug/demangle_helper.hpp>

namespace allscale { namespace data_item_manager {
    template <typename SplitRequirements, typename ProcessRequirements>
    struct task_requirements;

    struct task_requirements_base
    {
        using hierarchy_address = runtime::HierarchyAddress;

        virtual ~task_requirements_base()
        {}

        template <typename T>
        T& get()
        {
            HPX_ASSERT(dynamic_cast<T*>(this) != nullptr);

            return static_cast<T&>(*this);
        }

        template <typename T>
        T const& get() const
        {
            HPX_ASSERT(dynamic_cast<const T*>(this) != nullptr);

            return static_cast<T const&>(*this);
        }

        virtual void show() const = 0;

        virtual bool check_write_requirements(hierarchy_address const&) const = 0;
        virtual void get_missing_regions(hierarchy_address const&) = 0;
        virtual void add_allowance(hierarchy_address const&) const= 0;
        virtual void add_allowance_left(hierarchy_address const&)= 0;
        virtual void add_allowance_right(hierarchy_address const&)= 0;

        virtual hpx::future<void> acquire_split(hierarchy_address const& addr) const = 0;
        virtual hpx::future<void> acquire_process(hierarchy_address const& addr) const = 0;

        virtual void release_split() const = 0;
        virtual void release_process() const = 0;

        template <typename Archive>
        void serialize(Archive& ar, unsigned)
        {
        }

        HPX_SERIALIZATION_POLYMORPHIC_ABSTRACT(task_requirements_base);
    };

    template <typename SplitRequirements, typename ProcessRequirements>
    struct task_requirements : task_requirements_base
    {
        SplitRequirements split_requirements_;
        ProcessRequirements process_requirements_;

        task_requirements() {}

        explicit task_requirements(SplitRequirements&& split_requirements,
            ProcessRequirements&& process_requirements)
          : split_requirements_(std::move(split_requirements))
          , process_requirements_(std::move(process_requirements))
        {}

        void show() const final
        {
            std::cout << "Split:\n";
            data_item_manager::show(split_requirements_);
            std::cout << "Process:\n";
            data_item_manager::show(process_requirements_);
        }

        bool check_write_requirements(hierarchy_address const& addr) const final
        {
            return data_item_manager::check_write_requirements(addr, process_requirements_);
        }

        void get_missing_regions(hierarchy_address const& addr) final
        {
            data_item_manager::get_missing_regions(addr, process_requirements_);
        }

        void add_allowance(hierarchy_address const& addr) const final
        {
            data_item_manager::add_allowance(addr, process_requirements_);
        }

        void add_allowance_left(hierarchy_address const& addr) final
        {
            data_item_manager::add_allowance_left(addr, process_requirements_);
        }

        void add_allowance_right(hierarchy_address const& addr) final
        {
            data_item_manager::add_allowance_right(addr, process_requirements_);
        }

        hpx::future<void> acquire_split(hierarchy_address const& addr) const final
        {
            return data_item_manager::acquire(addr, split_requirements_);
        }
        hpx::future<void> acquire_process(hierarchy_address const& addr) const final
        {
            return data_item_manager::acquire(addr, process_requirements_);
        }

        void release_split() const final
        {
            // FIXME
        }
        void release_process() const final
        {
            // FIXME
        }

        template <typename, typename> friend
        struct ::hpx::serialization::detail::register_class_name;

        static std::string hpx_serialization_get_name_impl()
        {
            hpx::serialization::detail::register_class_name<
                task_requirements>::instance.instantiate();
            return hpx::util::debug::type_id<task_requirements>::typeid_.type_id();
        }

        std::string hpx_serialization_get_name() const final
        {
            return task_requirements::hpx_serialization_get_name_impl();
        }

        void load(hpx::serialization::input_archive& ar, unsigned n) final
        {
            ar & split_requirements_;
            ar & process_requirements_;
        }
        void save(hpx::serialization::output_archive& ar, unsigned n) const final
        {
            ar & split_requirements_;
            ar & process_requirements_;
        }
        HPX_SERIALIZATION_SPLIT_MEMBER()

    };
}}

#endif

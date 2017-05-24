
#ifndef ALLSCALE_DETAIL_WORK_ITEM_IMPL_HPP
#define ALLSCALE_DETAIL_WORK_ITEM_IMPL_HPP

#include <allscale/treeture.hpp>
#include <allscale/data_item.hpp>
#include <allscale/monitor.hpp>
#include <allscale/detail/unwrap_if.hpp>
#include <allscale/detail/work_item_impl_base.hpp>

#include <hpx/include/dataflow.hpp>
#include <hpx/include/serialization.hpp>
#include <hpx/util/assert.hpp>
#include <hpx/util/tuple.hpp>

#include <memory>
#include <utility>

namespace allscale { namespace detail {
	struct set_id {
		set_id(this_work_item::id& id) :
				old_id_(this_work_item::get_id()) {
			this_work_item::set_id(id);
		}

		~set_id() {
			this_work_item::set_id(old_id_);
		}

		this_work_item::id const& old_id_;
	};

    template <typename Archive, typename T>
    auto has_serialize(hpx::traits::detail::wrap_int)
      -> std::false_type;

    template <typename Archive, typename T>
    auto has_serialize(int)
      -> decltype(serialize(std::declval<Archive&>(), std::declval<T&>(), 0), std::true_type{});

    template <typename T>
    struct is_serializable
      : std::integral_constant<bool,
            decltype(has_serialize<hpx::serialization::input_archive, T>(0))::value &&
            decltype(has_serialize<hpx::serialization::output_archive, T>(0))::value &&
            std::is_default_constructible<T>::value &&
            !std::is_pointer<typename std::decay<T>::type>::value &&
            !std::is_reference<T>::value
        >
    {};

    template <typename T>
    struct is_closure_serializable;

    template <>
    struct is_closure_serializable<hpx::util::tuple<>> : std::true_type {};

    template <typename...Ts>
    struct is_closure_serializable<hpx::util::tuple<Ts...>>
      : hpx::util::detail::all_of<is_serializable<Ts>...>
    {};

    template<typename WorkItemDescription, typename Closure>
    struct work_item_impl
      : detail::work_item_impl_base
    {
        using result_type = typename WorkItemDescription::result_type;
        using closure_type = Closure;

        using data_item_descr = typename WorkItemDescription::data_item_variant;
        using data_item_type = typename allscale::data_item<data_item_descr>;

        static constexpr bool is_serializable = WorkItemDescription::ser_variant::activated &&
            is_closure_serializable<Closure>::value;

        // This ctor is called during serialization...
        work_item_impl(this_work_item::id const& id, treeture<result_type>&& tres, closure_type&& closure)
          : work_item_impl_base(id)
          , tres_(std::move(tres))
          , closure_(std::move(closure))
        {}

        template<typename ...Ts>
        work_item_impl(treeture<result_type> tres, Ts&&... vs)
         : tres_(std::move(tres)), closure_(std::forward<Ts>(vs)...)
        {
            HPX_ASSERT(tres_.valid());
        }

        std::shared_ptr<work_item_impl> shared_this()
        {
            return std::static_pointer_cast<work_item_impl>(
                this->shared_from_this());
        }

        const char* name() const {
            return WorkItemDescription::name();
        }

        treeture<void> get_treeture()
        {
            return tres_;
        }

        bool valid()
        {
            return tres_.valid() && bool(this->shared_from_this());
        }

        void on_ready(hpx::util::unique_function_nonser<void()> f)
        {
            typename hpx::traits::detail::shared_state_ptr_for<treeture<result_type>>::type state
                = hpx::traits::future_access<treeture<result_type>>::get_shared_state(tres_);

            state->set_on_completed(std::move(f));
        }

        bool can_split() const {
            return WorkItemDescription::can_split_variant::call(closure_);
        }

        template <typename Future>
        void finalize(std::shared_ptr<work_item_impl> this_, Future work_res)
        {
            monitor::signal(monitor::work_item_execution_finished,
                work_item(this_));

            typedef typename std::decay<Future>::type work_res_type;
            typename hpx::traits::detail::shared_state_ptr_for<work_res_type>::type state
                = hpx::traits::future_access<work_res_type>::get_shared_state(work_res);

            treeture<result_type> tres = tres_;
            state->set_on_completed(
                [tres, this_, state]() mutable
                {
                    set_id si(this_->id_);
                    detail::set_treeture(tres , state);
                    monitor::signal(monitor::work_item_result_propagated, work_item(this_));
                });
        }

        template<typename ...Ts>
		void do_process(Ts ...vs)
        {
            treeture<result_type> tres = tres_;

            std::shared_ptr < work_item_impl > this_(shared_this());
            monitor::signal(monitor::work_item_execution_started,
                work_item(this_));
            set_id si(this->id_);

            auto work_res =
                WorkItemDescription::process_variant::execute(
                    detail::unwrap_tuple(hpx::util::tuple<>(), std::move(vs)...));

            finalize(std::move(this_), std::move(work_res));
		}

        template<typename ...Ts>
        void do_split(Ts ...vs)
        {
            std::shared_ptr < work_item_impl > this_(shared_this());
            monitor::signal(monitor::work_item_execution_started, work_item(this_));
            set_id si(this->id_);

            auto work_res =
                WorkItemDescription::split_variant::execute(
                    unwrap_tuple(hpx::util::tuple<>(), std::move(vs)...));

            finalize(std::move(this_), std::move(work_res));
        }

        template<std::size_t ... Is>
        void process(hpx::util::detail::pack_c<std::size_t, Is...>)
        {
            void (work_item_impl::*f)(
                    typename hpx::util::decay<
                    decltype(hpx::util::get<Is>(closure_))>::type...
            ) = &work_item_impl::do_process;
            HPX_ASSERT(valid());
            hpx::dataflow(f, shared_this(),
                    std::move(hpx::util::get<Is>(closure_))...);
        }

        void process()
        {
            process(typename hpx::util::detail::make_index_pack<
                hpx::util::tuple_size<closure_type>::type::value>::type());
        }

        template<std::size_t ... Is>
        void split_impl(hpx::util::detail::pack_c<std::size_t, Is...>) {
            void (work_item_impl::*f)(
                    typename hpx::util::decay<
                    decltype(hpx::util::get<Is>(closure_))>::type...
            ) = &work_item_impl::do_split;
            HPX_ASSERT(valid());
            hpx::dataflow(f, shared_this(),
                    std::move(hpx::util::get<Is>(closure_))...);
        }

        template <typename WorkItemDescription_>
        typename std::enable_if<
            WorkItemDescription_::split_variant::valid
        >::type split_impl()
        {
            split_impl(typename hpx::util::detail::make_index_pack<
                hpx::util::tuple_size<closure_type>::type::value>::type());
        }

        template <typename WorkItemDescription_>
        typename std::enable_if<
            !WorkItemDescription_::split_variant::valid
        >::type split_impl()
        {
            process();
        }

        void split()
        {
            split_impl<WorkItemDescription>();
        }

        bool enqueue_remote() const
        {
            return is_serializable;
        }

        template <typename Closure_>
        typename std::enable_if<is_closure_serializable<Closure_>::value>::type
        preprocess(hpx::serialization::output_archive& ar)
        {
            ar & id_;
            ar & tres_;
            ar & closure_;
        }

        template <typename Closure_>
        typename std::enable_if<!is_closure_serializable<Closure_>::value>::type
        preprocess(hpx::serialization::output_archive& ar)
        {
            throw std::runtime_error("Attempt to serialize non serializable work_item");
        }

        void preprocess(hpx::serialization::output_archive& ar)
        {
            preprocess<closure_type>(ar);
        }

        HPX_SERIALIZATION_POLYMORPHIC_TEMPLATE_SEMIINTRUSIVE(work_item_impl);

        treeture<result_type> tres_;
        closure_type closure_;
    };

    template <typename Archive, typename WorkItemDescription, typename Closure>
    typename std::enable_if<
        work_item_impl<WorkItemDescription, Closure>::is_serializable
    >::type
    serialize(Archive& ar, work_item_impl<WorkItemDescription, Closure>& wi, unsigned)
    {
        ar & wi.id_;
        ar & wi.tres_;
        ar & wi.closure_;
    }

    template <typename Archive, typename WorkItemDescription, typename Closure>
    typename std::enable_if<
        !work_item_impl<WorkItemDescription, Closure>::is_serializable
    >::type
    serialize(Archive& ar, work_item_impl<WorkItemDescription, Closure>& wi, unsigned)
    {
        throw std::runtime_error("Attempt to serialize non serializable work_item");
    }

    template <typename WorkItemDescription, typename Closure>
    typename std::enable_if<
        work_item_impl<WorkItemDescription, Closure>::is_serializable,
        work_item_impl<WorkItemDescription, Closure>*
    >::type
    load_work_item_impl(
        hpx::serialization::input_archive& ar,
        work_item_impl<WorkItemDescription, Closure>* /*unused*/)
    {
        typedef work_item_impl<WorkItemDescription, Closure> work_item_type;

        this_work_item::id id;
        treeture<typename work_item_type::result_type> tres;
        typename work_item_type::closure_type closure;

        ar & id;
        ar & tres;
        ar & closure;

        return new work_item_type(id, std::move(tres), std::move(closure));
    }

    template <typename WorkItemDescription, typename Closure>
    typename std::enable_if<
        !work_item_impl<WorkItemDescription, Closure>::is_serializable,
        work_item_impl<WorkItemDescription, Closure>*
    >::type
    load_work_item_impl(
        hpx::serialization::input_archive& ar,
        work_item_impl<WorkItemDescription, Closure>* /*unused*/)
    {
        throw std::runtime_error("Attempt to serialize non serializable work_item");
        return nullptr;
    }
}}

HPX_SERIALIZATION_WITH_CUSTOM_CONSTRUCTOR_TEMPLATE(
    (template <typename WorkItemDescription, typename Closure>),
    (allscale::detail::work_item_impl<WorkItemDescription, Closure>),
    allscale::detail::load_work_item_impl
);

HPX_TRAITS_NONINTRUSIVE_POLYMORPHIC_TEMPLATE(
    (template <typename WorkItemDescription, typename Closure>),
    (allscale::detail::work_item_impl<WorkItemDescription, Closure>));
HPX_SERIALIZATION_REGISTER_CLASS_NAME_TEMPLATE(
    (template <typename WorkItemDescription, typename Closure>),
    (allscale::detail::work_item_impl<WorkItemDescription, Closure>),
    WorkItemDescription::name());

template <typename WorkItemDescription, typename Closure>
hpx::serialization::detail::register_class<allscale::detail::work_item_impl<WorkItemDescription, Closure>>
allscale::detail::work_item_impl<WorkItemDescription, Closure>::hpx_register_class_instance;

#endif

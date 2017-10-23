
#ifndef ALLSCALE_DETAIL_WORK_ITEM_IMPL_HPP
#define ALLSCALE_DETAIL_WORK_ITEM_IMPL_HPP

#include <allscale/treeture.hpp>
#include <allscale/monitor.hpp>
#include <allscale/data_item_manager.hpp>
#include <allscale/detail/unwrap_if.hpp>
#include <allscale/detail/work_item_impl_base.hpp>

#include <hpx/include/serialization.hpp>
#include <hpx/util/assert.hpp>
#include <hpx/util/tuple.hpp>
#include <hpx/lcos/when_all.hpp>

#include <memory>
#include <utility>

namespace allscale { namespace detail {
	struct set_id {
		set_id(this_work_item::id& id)
          :	old_id_(this_work_item::get_id_ptr()) {
			this_work_item::set_id(id);
		}

		~set_id() {
            if (old_id_)
                this_work_item::set_id(*old_id_);
		}

		this_work_item::id* old_id_;
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

    template <typename Leases, std::size_t... Is>
    inline void release(hpx::util::detail::pack_c<std::size_t, Is...>, Leases const& leases)
    {
        int sequencer[] = {
            (allscale::data_item_manager::release(hpx::util::get<Is>(leases)), 0)...
        };
    }

    inline void release(hpx::util::detail::pack_c<std::size_t>, hpx::util::tuple<> const&)
    {
    }

    template <typename Requirements, std::size_t... Is>
    inline auto get_leases(hpx::util::detail::pack_c<std::size_t, Is...>, Requirements const& reqs)
     -> hpx::util::tuple<
            typename hpx::util::decay<decltype(
                allscale::data_item_manager::acquire(hpx::util::get<Is>(reqs)) )>::type...
        >
    {
        return hpx::util::make_tuple(
            allscale::data_item_manager::acquire(hpx::util::get<Is>(reqs))...
        );
    }

    template<typename WorkItemDescription, typename Closure>
    struct work_item_impl
      : detail::work_item_impl_base
    {
        using result_type = typename WorkItemDescription::result_type;
        using closure_type = Closure;


        static constexpr bool is_serializable = WorkItemDescription::ser_variant::activated; /*&&
            is_closure_serializable<Closure>::value;*/

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

        work_item_impl(this_work_item::id const& id, hpx::shared_future<void> dep, treeture<result_type>&& tres, closure_type&& closure)
          : work_item_impl_base(id)
          , tres_(std::move(tres))
          , closure_(std::move(closure))
          , dep_(dep)
        {}

        template<typename ...Ts>
        work_item_impl(hpx::shared_future<void> dep, treeture<result_type> tres, Ts&&... vs)
         : tres_(std::move(tres)), closure_(std::forward<Ts>(vs)...), dep_(std::move(dep))
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
            typename hpx::traits::detail::shared_state_ptr_for<treeture<result_type>>::type const& state
                = hpx::traits::future_access<treeture<result_type>>::get_shared_state(tres_);

            state->set_on_completed(std::move(f));
        }

        bool can_split() const {
            return WorkItemDescription::can_split_variant::call(closure_);
        }

        template <typename Variant>
        struct acquire_result
        {
            typedef
                decltype(Variant::get_requirements(std::declval<Closure const&>()))
                reqs;
            typedef typename hpx::util::detail::make_index_pack<
                hpx::util::tuple_size<reqs>::type::value>::type pack;

            typedef decltype(get_leases(std::declval<pack>(), std::declval<reqs>())) type;
        };

        template <typename Variant>
        auto acquire(decltype(&Variant::template get_requirements<Closure>))
         -> typename acquire_result<Variant>::type
        {
            typename acquire_result<Variant>::pack pack;
            return get_leases(pack, Variant::get_requirements(closure_));
        }

        // Overload for non templates...
        template <typename Variant>
        auto acquire(decltype(&Variant::get_requirements))
         -> typename acquire_result<Variant>::type
        {
            typename acquire_result<Variant>::pack pack;
            return get_leases(pack, Variant::get_requirements(closure_));
        }

        template <typename Variant>
        hpx::util::tuple<> acquire(...)
        {
            return hpx::util::tuple<>();
        }

        template <typename Future, typename Leases>
        typename std::enable_if<!std::is_same<Future, hpx::util::unused_type>::value>::type
        finalize(
            std::shared_ptr<work_item_impl> this_, Future work_res, Leases leases)
        {
            monitor::signal(monitor::work_item_execution_finished,
                work_item(this_));

            typedef typename std::decay<Future>::type work_res_type;
            typedef typename hpx::traits::detail::shared_state_ptr_for<work_res_type>::type
                shared_state_type;
            shared_state_type state
                = hpx::traits::future_access<work_res_type>::get_shared_state(work_res);

            state->set_on_completed(
                hpx::util::deferred_call([](auto this_, auto state, Leases leases)
                {
                    typename hpx::util::detail::make_index_pack<
                        hpx::util::tuple_size<Leases>::type::value>::type pack;
                    release(pack, leases);

                    set_id si(this_->id_);
                    detail::set_treeture(this_->tres_ , state);
                    monitor::signal(monitor::work_item_result_propagated,
                        work_item(std::move(this_)));
                },
                std::move(this_), state, std::move(leases)));
        }

        template <typename Leases>
        void finalize(
            std::shared_ptr<work_item_impl>&& this_, hpx::util::unused_type, Leases leases)
        {
            monitor::signal(monitor::work_item_execution_finished,
                work_item(this_));

            typename hpx::util::detail::make_index_pack<
                hpx::util::tuple_size<Leases>::type::value>::type pack;
            release(pack, leases);

            set_id si(this_->id_);
            tres_.set_value(hpx::util::unused_type{});
            monitor::signal(monitor::work_item_result_propagated,
                work_item(std::move(this_)));
        }

        template<typename Leases, typename ...Ts>
		void do_process(Leases leases, Ts ...vs)
        {
            std::shared_ptr < work_item_impl > this_(shared_this());
            monitor::signal(monitor::work_item_execution_started,
                work_item(this_));
            set_id si(this->id_);

            auto work_res =
                WorkItemDescription::process_variant::execute(
                    detail::unwrap_tuple(hpx::util::tuple<>(), std::move(vs)...));

            finalize(std::move(this_), std::move(work_res), std::move(leases));
		}

        template <typename T>
        void set_children(treeture<T> const& tre)
        {
            if (tre.valid())
                tres_.set_children(tre.get_left_child(), tre.get_right_child());
        }

        template <typename T>
        void set_children(T const&)
        {
            // null-op as fallback...
        }

        template<typename ...Ts, typename Leases>
		void do_split(Leases leases, Ts ...vs)
        {
            std::shared_ptr < work_item_impl > this_(shared_this());
            monitor::signal(monitor::work_item_execution_started, work_item(this_));

            set_id si(this->id_);

            auto work_res =
                WorkItemDescription::split_variant::execute(
                    unwrap_tuple(hpx::util::tuple<>(), std::move(vs)...));
            set_children(work_res);

            finalize(std::move(this_), std::move(work_res), std::move(leases));
        }

        template <typename ProcessVariant, typename Leases, std::size_t... Is>
        void get_deps(executor_type& exec, hpx::util::detail::pack_c<std::size_t, Is...>, Leases leases,
            decltype(&ProcessVariant::template deps<Closure>))
        {
            typedef hpx::shared_future<void> deps_type;

            deps_type deps;
            if (dep_.valid() && !dep_.is_ready())
            {
                deps = hpx::when_all(ProcessVariant::deps(closure_), dep_).share();
            }
            else
            {
                deps = ProcessVariant::deps(closure_);
            }

            typename hpx::traits::detail::shared_state_ptr_for<deps_type>::type const& state
                = hpx::traits::future_access<deps_type>::get_shared_state(deps);

            auto this_ = shared_this();
            state->set_on_completed(
                [exec, state, this_, leases]() mutable
                {
//                     state->get_result();
                    void (work_item_impl::*f)(
                            Leases,
                            typename hpx::util::decay<
                            decltype(hpx::util::get<Is>(closure_))>::type...
                    ) = &work_item_impl::do_process;

                    hpx::apply(exec, f, this_,
                        std::move(leases), std::move(hpx::util::get<Is>(this_->closure_))...);
                });
        }

        template <typename ProcessVariant, typename Leases, std::size_t... Is>
        void get_deps(executor_type& exec, hpx::util::detail::pack_c<std::size_t, Is...>, Leases leases, ...)
        {
            auto this_ = shared_this();
            if (dep_.valid() && !dep_.is_ready())
            {
                typedef hpx::shared_future<void> deps_type;

                typename hpx::traits::detail::shared_state_ptr_for<deps_type>::type const& state
                    = hpx::traits::future_access<deps_type>::get_shared_state(dep_);
                state->set_on_completed(
                    [exec, state, this_, leases]() mutable
                    {
                        void (work_item_impl::*f)(
                                Leases,
                                typename hpx::util::decay<
                                decltype(hpx::util::get<Is>(closure_))>::type...
                        ) = &work_item_impl::do_process;

                        hpx::apply(exec, f, this_,
                            std::move(leases), std::move(hpx::util::get<Is>(this_->closure_))...);
                    }
                );
            }
            else
            {
                do_process(
                    std::move(leases), std::move(hpx::util::get<Is>(closure_))...);
            }
        }

        template<std::size_t ... Is>
        void process(executor_type& exec, hpx::util::detail::pack_c<std::size_t, Is...> pack)
        {
            HPX_ASSERT(valid());
            auto reqs = acquire<typename WorkItemDescription::process_variant>(nullptr);

            typedef typename hpx::util::decay<decltype(hpx::util::unwrap(reqs))>::type reqs_type;

            auto this_ = shared_this();
            hpx::dataflow(exec,
                hpx::util::unwrapping([this_, exec, pack](reqs_type reqs) mutable
                {
                    this_->template get_deps<typename WorkItemDescription::process_variant>(exec, pack, reqs, nullptr);
                })
              , std::move(reqs));
        }

        void process(executor_type& exec)
        {
            process(exec, typename hpx::util::detail::make_index_pack<
                hpx::util::tuple_size<closure_type>::type::value>::type());
        }

        template<std::size_t ... Is>
        void split_impl(executor_type& exec, hpx::util::detail::pack_c<std::size_t, Is...>) {
            auto reqs = acquire<typename WorkItemDescription::split_variant>(nullptr);

            typedef typename hpx::util::decay<decltype(hpx::util::unwrap(reqs))>::type reqs_type;
            auto this_ = shared_this();
            hpx::dataflow(exec,
                hpx::util::unwrapping([this_](reqs_type reqs) mutable
                {
                    HPX_ASSERT(this_->valid());
                    this_->do_split(std::move(reqs), std::move(hpx::util::get<Is>(this_->closure_))...);
                })
              , std::move(reqs));
        }

        template <typename WorkItemDescription_>
        typename std::enable_if<
            WorkItemDescription_::split_variant::valid
        >::type split_impl(executor_type& exec)
        {
            split_impl(exec, typename hpx::util::detail::make_index_pack<
                hpx::util::tuple_size<closure_type>::type::value>::type());
        }

        template <typename WorkItemDescription_>
        typename std::enable_if<
            !WorkItemDescription_::split_variant::valid
        >::type split_impl(executor_type& exec)
        {
            process(exec);
        }

        void split(executor_type& exec)
        {
            split_impl<WorkItemDescription>(exec);
        }

        bool enqueue_remote() const
        {
        	//std::cout<<"eqn remote called  " <<  WorkItemDescription::ser_variant::activated << "   " <<  is_closure_serializable<Closure>::value << std::endl;;
            return is_serializable;
        }

        HPX_SERIALIZATION_POLYMORPHIC_TEMPLATE_SEMIINTRUSIVE(work_item_impl);

        treeture<result_type> tres_;
        closure_type closure_;
        hpx::shared_future<void> dep_;
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
        ar & wi.dep_;
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
        hpx::shared_future<void> dep;

        ar & id;
        ar & tres;
        ar & closure;
        ar & dep;

        return new work_item_type(id, std::move(dep), std::move(tres), std::move(closure));
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

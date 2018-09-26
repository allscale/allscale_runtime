
#ifndef ALLSCALE_DETAIL_WORK_ITEM_IMPL_HPP
#define ALLSCALE_DETAIL_WORK_ITEM_IMPL_HPP

#include <allscale/treeture.hpp>
#include <allscale/monitor.hpp>
#include <allscale/data_item_requirement.hpp>
#include <allscale/detail/work_item_impl_base.hpp>
#include <allscale/utils/serializer/tuple.h>

#include <hpx/include/serialization.hpp>
#include <hpx/include/parallel_execution.hpp>
#include <hpx/util/assert.hpp>
#include <hpx/util/annotated_function.hpp>
#include <hpx/util/tuple.hpp>
#include <hpx/lcos/when_all.hpp>

#include <memory>
#include <utility>

namespace allscale {
namespace utils {
	namespace detail {

        namespace detail {

            template<std::size_t Pos>
            struct tuple_for_each_helper {
                template<typename Op, typename ... Args>
                void operator()(const Op& op, hpx::util::tuple<Args...>& tuple) {
                    tuple_for_each_helper<Pos-1>()(op,tuple);
                    op(hpx::util::get<Pos-1>(tuple));
                }
                template<typename Op, typename ... Args>
                void operator()(const Op& op, const hpx::util::tuple<Args...>& tuple) {
                    tuple_for_each_helper<Pos-1>()(op,tuple);
                    op(hpx::util::get<Pos-1>(tuple));
                }
            };

            template<>
            struct tuple_for_each_helper<0> {
                template<typename Op, typename ... Args>
                void operator()(const Op&, const hpx::util::tuple<Args...>&) {
                    // nothing
                }
            };

        }

        /**
         * A utility to apply an operator on all elements of a tuple in order.
         *
         * @param tuple the (mutable) tuple
         * @param op the operator to be applied
         */
        template<typename Op, typename ... Args>
        void forEach(hpx::util::tuple<Args...>& tuple, const Op& op) {
            detail::tuple_for_each_helper<sizeof...(Args)>()(op,tuple);
        }

        /**
         * A utility to apply an operator on all elements of a tuple in order.
         *
         * @param tuple the (constant) tuple
         * @param op the operator to be applied
         */
        template<typename Op, typename ... Args>
        void forEach(const hpx::util::tuple<Args...>& tuple, const Op& op) {
            detail::tuple_for_each_helper<sizeof...(Args)>()(op,tuple);
        }

		// a utility assisting in loading non-trivial tuples from stream

		template<std::size_t pos, typename ... Args>
		struct hpx_load_helper {

			using inner = hpx_load_helper<pos-1,Args...>;

			template<typename ... Cur>
			hpx::util::tuple<Args...> operator()(ArchiveReader& in, Cur&& ... cur) const {
				using cur_t = std::remove_reference_t<decltype(hpx::util::get<sizeof...(Cur)>(std::declval<hpx::util::tuple<Args...>>()))>;
				return inner{}(in,std::move(cur)...,in.read<cur_t>());
			}

		};

		template<typename ... Args>
		struct hpx_load_helper<0,Args...> {

            hpx::util::tuple<Args...> operator()(ArchiveReader&, Args&& ... args) const {
				return hpx::util::make_tuple(std::move(args)...);
			}

		};

	}


	/**
	 * Add support for serializing / de-serializing tuples of non-trivial element types.
	 */
    template <typename T>
    struct tuple_serializer;

	template<typename ... Args>
	struct tuple_serializer<hpx::util::tuple<Args...> > {

		static hpx::util::tuple<Args...> load(ArchiveReader& reader) {
			return detail::hpx_load_helper<sizeof...(Args),Args...>{}(reader);
		}


		static void store(ArchiveWriter& writer, const hpx::util::tuple<Args...>& value) {
            allscale::utils::detail::forEach(value,[&](const auto& cur){
				writer.write(cur);
			});
		}
	};

} // end namespace utils
} // end namespace allscale

namespace allscale { namespace detail {
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

    template<typename T, typename SharedState>
    inline void set_treeture(treeture<T>& t, SharedState& s) {
        t.set_value(std::move(*s->get_result()));
    }

    template<typename SharedState>
    inline void set_treeture(treeture<void>& t, SharedState& s) {
    // 	f.get(); // exception propagation
        s->get_result_void();
        t.set_value(hpx::util::unused_type{});
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
        work_item_impl(task_id const& id, closure_type&& closure)
          : work_item_impl_base(id)
          , tres_(treeture_init)
          , closure_(std::move(closure))
        {}

        work_item_impl(task_id const& id, hpx::shared_future<void> dep, closure_type&& closure)
          : work_item_impl_base(id)
          , tres_(treeture_init)
          , closure_(std::move(closure))
          , dep_(std::move(dep))
        {}

        // used for serialization...
        work_item_impl(task_id const& id, hpx::shared_future<void> dep, treeture<result_type> tres, closure_type&& closure)
          : work_item_impl_base(id)
          , tres_(std::move(tres))
          , closure_(std::move(closure))
          , dep_(std::move(dep))
        {}

        std::shared_ptr<work_item_impl> shared_this()
        {
            return std::static_pointer_cast<work_item_impl>(
                this->shared_from_this());
        }

        const char* name() const final {
            return WorkItemDescription::name();
        }

        treeture<void> get_void_treeture() const final
        {
            return tres_;
        }

        treeture<result_type> get_treeture() const
        {
            return tres_;
        }

        bool valid() final
        {
            return tres_.valid() && bool(this->shared_from_this());
        }

        void on_ready(hpx::util::unique_function_nonser<void()> f) final
        {
            typename hpx::traits::detail::shared_state_ptr_for<treeture<result_type>>::type const& state
                = hpx::traits::future_access<treeture<result_type>>::get_shared_state(tres_);

            state->set_on_completed(std::move(f));
        }

        bool can_split() const final {
            return WorkItemDescription::can_split_variant::call(closure_) &&
                WorkItemDescription::split_variant::valid;
        }

        template <typename Variant>
        auto get_requirements(decltype(&Variant::template get_requirements<Closure>)) const
         -> decltype(Variant::get_requirements(std::declval<Closure const&>()))
        {
            return Variant::get_requirements(closure_);
        }

        template <typename Variant>
        auto get_requirements(decltype(&Variant::get_requirements)) const
         -> decltype(Variant::get_requirements(std::declval<Closure const&>()))
        {
            return Variant::get_requirements(closure_);
        }

        template <typename Variant>
        hpx::util::tuple<> get_requirements(...) const
        {
            return hpx::util::tuple<>();
        }

        template <typename Future>
        typename std::enable_if<
            hpx::traits::is_future<Future>::value ||
            traits::is_treeture<Future>::value
        >::type
        finalize(
            std::shared_ptr<work_item_impl> this_, Future && work_res, task_requirements&& reqs, bool is_split)
        {
            if(is_split)
            {
                monitor::signal(monitor::work_item_split_execution_finished,
                    work_item(this_));
            }
            else
            {
                monitor::signal(monitor::work_item_process_execution_finished, work_item(this_));
            }


            typedef typename std::decay<Future>::type work_res_type;
            typedef typename hpx::traits::detail::shared_state_ptr_for<work_res_type>::type
                shared_state_type;
            shared_state_type state
                = hpx::traits::future_access<work_res_type>::get_shared_state(work_res);

            state->set_on_completed(
                hpx::util::deferred_call([is_split](auto this_, auto state, task_requirements&& reqs)
                {
                    if(is_split)
                    {
                        reqs->release_split();
                    }
                    else
                    {
                        reqs->release_process();
                    }

                    this_work_item::set s(*this_);
                    detail::set_treeture(this_->tres_ , state);
                    monitor::signal(monitor::work_item_result_propagated,
                        work_item(std::move(this_)));
                },
                std::move(this_), std::move(state), std::move(reqs)));
        }

        template <typename Future>
        typename std::enable_if<
            !hpx::traits::is_future<Future>::value &&
            !traits::is_treeture<Future>::value
        >::type
        finalize(
            std::shared_ptr<work_item_impl> this_, Future && work_res, task_requirements&& reqs, bool is_split)
        {
            if(is_split)
            {
                monitor::signal(monitor::work_item_split_execution_finished,
                    work_item(this_));
                reqs->release_split();
            }
            else
            {
                monitor::signal(monitor::work_item_process_execution_finished, work_item(this_));
                reqs->release_process();
            }

            this_work_item::set s(*this_);
            tres_.set_value(std::forward<Future>(work_res));
            monitor::signal(monitor::work_item_result_propagated,
                work_item(std::move(this_)));
        }

        template <typename Closure_>
        typename std::enable_if<
            !std::is_same<
                decltype(WorkItemDescription::process_variant::execute(std::declval<Closure_ const&>())),
                void
            >::value
        >::type
		do_process(task_requirements&& reqs)
        {
            std::shared_ptr < work_item_impl > this_(shared_this());
            monitor::signal(monitor::work_item_process_execution_started,
                work_item(this_));
            this_work_item::set s(*this_);

            auto work_res =
                WorkItemDescription::process_variant::execute(closure_);

            finalize(std::move(this_), std::move(work_res), std::move(reqs), false);
		}

        template <typename Closure_>
        typename std::enable_if<
            std::is_same<
                decltype(WorkItemDescription::process_variant::execute(std::declval<Closure_ const&>())),
                void
            >::value
        >::type
		do_process(task_requirements&& reqs)
        {
            std::shared_ptr < work_item_impl > this_(shared_this());
            monitor::signal(monitor::work_item_process_execution_started,
                work_item(this_));
            this_work_item::set s(*this_);

            WorkItemDescription::process_variant::execute(closure_);

            finalize(std::move(this_), hpx::util::unused_type(), std::move(reqs), false);
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

		void do_split(task_requirements&& reqs)
        {
            std::shared_ptr < work_item_impl > this_(shared_this());
            monitor::signal(monitor::work_item_split_execution_started, work_item(this_));

            this_work_item::set s(*this_);

            auto work_res =
                WorkItemDescription::split_variant::execute(closure_);
            set_children(work_res);

            finalize(std::move(this_), std::move(work_res), std::move(reqs), true);
        }

        template <typename ProcessVariant>
        void get_deps(executor_type& exec, task_requirements&& reqs, decltype(&ProcessVariant::template deps<Closure>))
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

            typename hpx::traits::detail::shared_state_ptr_for<deps_type>::type state
                = hpx::traits::future_access<deps_type>::get_shared_state(deps);

            auto this_ = shared_this();
            state->set_on_completed(
                [&exec, state = std::move(state), this_ = std::move(this_), reqs = std::move(reqs)]() mutable
                {
                    void (work_item_impl::*f)(
                        task_requirements&&
                    ) = &work_item_impl::do_process<Closure>;

                    hpx::parallel::execution::post(
                        exec, hpx::util::annotated_function(
                            hpx::util::deferred_call(f, std::move(this_), std::move(reqs)),
                            "allscale::work_item::do_process"));
                });
        }

        template <typename ProcessVariant>
        void get_deps(executor_type& exec, task_requirements&& reqs, ...)
        {
            auto this_ = shared_this();
            if (dep_.valid() && !dep_.is_ready())
            {
                typedef hpx::shared_future<void> deps_type;

                typename hpx::traits::detail::shared_state_ptr_for<deps_type>::type state
                    = hpx::traits::future_access<deps_type>::get_shared_state(dep_);
                state->set_on_completed(
                    [&exec, state = std::move(state), this_ = std::move(this_), reqs = std::move(reqs)]() mutable
                    {
                        void (work_item_impl::*f)(
                            task_requirements&&
                        ) = &work_item_impl::do_process<Closure>;

                        hpx::parallel::execution::post(
                            exec, hpx::util::annotated_function(
                                hpx::util::deferred_call(f, std::move(this_), std::move(reqs)),
                                "allscale::work_item::do_process"));
                    }
                );
            }
            else
            {
                hpx::util::annotate_function("allscale::work_item::do_process_sync");
                do_process<Closure>(std::move(reqs));
            }
        }

        void process(executor_type& exec, task_requirements&& reqs) final
        {
            hpx::util::annotate_function("allscale::work_item::process");
            get_deps<typename WorkItemDescription::process_variant>(
                exec, std::move(reqs), nullptr);
        }

        template <typename WorkItemDescription_>
        typename std::enable_if<
            WorkItemDescription_::split_variant::valid
        >::type split(executor_type& exec, task_requirements&& reqs)
        {
            if (id().is_right())
            {
                hpx::util::annotate_function("allscale::work_item::do_split_sync");
                do_split(std::move(reqs));
            }
            else
            {
                auto this_ = shared_this();
                hpx::parallel::execution::post(
                    exec, hpx::util::annotated_function(
                        [this_ = std::move(this_), reqs = std::move(reqs)]() mutable
                        {
                            this_->do_split(std::move(reqs));
                        },
                        "allscale::work_item::do_process"));
            }
        }

        template <typename WorkItemDescription_>
        typename std::enable_if<
            !WorkItemDescription_::split_variant::valid
        >::type split(executor_type&, task_requirements&& reqs)
        {
            throw std::logic_error(
                "Calling split on a work item without valid split variant");
        }

        void split(executor_type& exec, task_requirements&& reqs) final
        {
            split<WorkItemDescription>(exec, std::move(reqs));
        }

        virtual std::unique_ptr<data_item_manager::task_requirements_base>
            get_task_requirements() const override
        {
            using split_requirements =
                typename hpx::util::decay<
                    decltype(
                        detail::merge_data_item_reqs(
                            get_requirements<typename WorkItemDescription::split_variant>(nullptr)
                        )
                    )
                >::type;

            using process_requirements =
                typename hpx::util::decay<
                    decltype(
                        detail::merge_data_item_reqs(
                            get_requirements<typename WorkItemDescription::process_variant>(nullptr)
                        )
                    )
                >::type;

            return std::unique_ptr<data_item_manager::task_requirements_base>(
                new data_item_manager::task_requirements<split_requirements, process_requirements>(
                    detail::merge_data_item_reqs(
                        get_requirements<typename WorkItemDescription::split_variant>(nullptr)
                    ),
                    detail::merge_data_item_reqs(
                        get_requirements<typename WorkItemDescription::process_variant>(nullptr)
                    )
                )
            );
        }

        bool enqueue_remote() const final
        {
            return is_serializable;
        }

        HPX_SERIALIZATION_POLYMORPHIC_TEMPLATE_SEMIINTRUSIVE(work_item_impl);

        treeture<result_type> tres_;
        closure_type closure_;
        hpx::shared_future<void> dep_;
    };

    template <typename WorkItemDescription, typename Closure>
    void serialize(hpx::serialization::input_archive& ar, work_item_impl<WorkItemDescription, Closure>& wi, unsigned)
    {
        HPX_ASSERT(false);
    }

    template <typename WorkItemDescription, typename Closure>
    typename std::enable_if<
        work_item_impl<WorkItemDescription, Closure>::is_serializable
    >::type
    serialize(hpx::serialization::output_archive& ar, work_item_impl<WorkItemDescription, Closure>& wi, unsigned)
    {
        typedef work_item_impl<WorkItemDescription, Closure> work_item_type;
        using closure_type = typename work_item_type::closure_type;
        ar & wi.id_;
        ar & wi.num_children;
        ar & wi.tres_;
        allscale::utils::ArchiveWriter writer(ar);
        allscale::utils::tuple_serializer<closure_type>::store(writer, wi.closure_);
        ar & wi.dep_;
    }

    template <typename WorkItemDescription, typename Closure>
    typename std::enable_if<
        !work_item_impl<WorkItemDescription, Closure>::is_serializable
    >::type
    serialize(hpx::serialization::output_archive&, work_item_impl<WorkItemDescription, Closure>&, unsigned)
    {
        std::cerr << "Attempt to serialize non serializable work_item: " <<
            WorkItemDescription::name() << "\n";
        std::abort();
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
        using closure_type = typename work_item_type::closure_type;

        std::uint8_t num_children;
        task_id id;
        treeture<typename work_item_type::result_type> tres;
        hpx::shared_future<void> dep;

        ar & id;
        ar & num_children;
        ar & tres;
        allscale::utils::ArchiveReader reader(ar);
        closure_type closure = allscale::utils::tuple_serializer<closure_type>::load(reader);
        ar & dep;

        auto res = new work_item_type(id, std::move(dep), std::move(tres), std::move(closure));

        res->num_children = num_children;

        return res;
    }

    template <typename WorkItemDescription, typename Closure>
    typename std::enable_if<
        !work_item_impl<WorkItemDescription, Closure>::is_serializable,
        work_item_impl<WorkItemDescription, Closure>*
    >::type
    load_work_item_impl(
        hpx::serialization::input_archive&,
        work_item_impl<WorkItemDescription, Closure>* /*unused*/)
    {
        std::cerr << "Attempt to serialize non serializable work_item: " <<
            WorkItemDescription::name() << "\n";
        std::abort();
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

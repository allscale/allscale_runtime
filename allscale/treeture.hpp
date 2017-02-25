
#ifndef ALLSCALE_TREETURE_HPP
#define ALLSCALE_TREETURE_HPP

#include <allscale/components/treeture.hpp>
#include <allscale/traits/is_treeture.hpp>

#include <hpx/include/future.hpp>
#include <hpx/include/lcos.hpp>

#include <utility>

namespace allscale
{

    // a naive version of a task reference, TODO: improve
    struct task_reference {

        // a function to wait for the completion of the referenced task
        const std::function<void()> wait;

        // a function to obtain a reference to the left child (which might only exist in the future)
        const std::function<task_reference()> get_left_child;

        // a function to obtain a reference to the right child (which might only exist in the future)
        const std::function<task_reference()> get_right_child;

    };



    template <typename T>
    struct treeture
      : hpx::components::client_base<treeture<T>, components::treeture<T>>
    {
        using base_type = hpx::components::client_base<treeture<T>, components::treeture<T>>;
        using result_type = T;
        using future_type = hpx::future<T>;

        using set_value_action =
            typename components::treeture<T>::set_value_action;
        using get_future_action =
            typename components::treeture<T>::get_future_action;

        treeture()
        {}

        treeture(hpx::id_type loc)
          : base_type(hpx::new_<components::treeture<T>>(loc))
        {
            HPX_ASSERT(this->valid());
        }

        template <
            typename F,
            typename Enable =
                typename std::enable_if<
                    hpx::traits::is_future<F>::value
                 && !std::is_same<typename hpx::util::decay<F>::type, treeture>::value
                 && !std::is_same<typename hpx::util::decay<F>::type, hpx::future<treeture>>::value
                 && !std::is_same<typename hpx::util::decay<F>::type, hpx::future<hpx::id_type>>::value
                >::type
        >
        explicit treeture(F f)
          : base_type(hpx::new_<components::treeture<T>>(hpx::find_here()))
        {
            HPX_ASSERT(this->valid());
            f.then(
                [this](F f)
                {
                    hpx::apply<set_value_action>(
                        this->get_id(), f.get()
                    );
                }
            );
            HPX_ASSERT(this->valid());
        }

        template <
            typename U,
            typename Enable =
                typename std::enable_if<
                    !hpx::traits::is_future<U>::value
                 && !std::is_same<typename hpx::util::decay<U>::type, treeture>::value
                 && !std::is_same<typename hpx::util::decay<U>::type, hpx::future<treeture>>::value
                 && !std::is_same<typename hpx::util::decay<U>::type, hpx::future<hpx::id_type>>::value
                >::type
        >
        explicit treeture(U && u)
          : base_type(hpx::new_<components::treeture<T>>(hpx::find_here(), std::forward<U>(u)))
        {
            HPX_ASSERT(this->valid());
        }

        treeture(treeture && o)
          : base_type(std::move(o))
        {
            HPX_ASSERT(this->valid());
        }

        treeture(treeture const& o)
          : base_type(o)
        {
            HPX_ASSERT(this->valid());
        }

        explicit treeture(hpx::future<treeture> && f)
          : base_type(std::move(f))
        {
            HPX_ASSERT(this->valid());
            HPX_ASSERT(this->get_id().valid());
        }

        explicit treeture(hpx::future<hpx::id_type> && f)
          : base_type(std::move(f))
        {
            HPX_ASSERT(this->valid());
        }

        treeture& operator=(treeture&& other) {
            base_type::operator=(std::move(other));
            return *this;
        }

        void set_value(T && t)
        {
            HPX_ASSERT(this->valid());
            hpx::apply<set_value_action>(
                this->get_id(), std::move(t)
            );
//             this->reset();
        }

        hpx::future<T> get_future()
        {
            HPX_ASSERT(this->valid());
//             hpx::shared_future<hpx::id_type> g = gid_;
//             this->reset();
            return hpx::async<get_future_action>(this->get_id());
        }

        T get_result()
        {
            HPX_ASSERT(this->valid());
            return get_future().get();
        }

        void wait()
        {
            // TODO: this fails in the fine_grained cases since the future has been retrieved before
            // How else can I wait for the completion of the task?            

            get_future().wait();     // TODO: provide replacement
        }

        task_reference to_task_reference() const {
            treeture copy = *this;
            return task_reference{
                // wait for the task completion
                [copy]() mutable { copy.wait(); },
                // get the left child - TODO: provide actual support for this
                [copy](){ return copy.to_task_reference(); },
                // get the right child - TODO: provide actual support for this
                [copy](){ return copy.to_task_reference(); }
            };
        }

        operator task_reference() const {
            return to_task_reference();
        }

        task_reference get_left_child() const {
            return to_task_reference().get_left_child();
        }

        task_reference get_right_child() const {
            return to_task_reference().get_right_child();
        }

//         template <typename Archive>
//         void load(Archive & ar, unsigned)
//         {
//             ar & hpx::serialization::base_object<base_type>(*this);
//             HPX_ASSERT(this->valid());
//         }
//
//         template <typename Archive>
//         void save(Archive & ar, unsigned) const
//         {
//             HPX_ASSERT(this->valid());
//             ar & hpx::serialization::base_object<base_type>(*this);
//             HPX_ASSERT(this->valid());
//         }
//
//         HPX_SERIALIZATION_SPLIT_MEMBER();
    };

    namespace traits
    {
        template <typename T>
        struct is_treeture< ::allscale::treeture<T>>
          : std::true_type
        {};
    }
}

#define ALLSCALE_REGISTER_TREETURE_TYPE_(T, NAME)                               \
    using NAME =                                                                \
        ::hpx::components::managed_component<                                   \
            ::allscale::components::treeture< T >                               \
        >;                                                                      \
    HPX_REGISTER_COMPONENT(NAME)                                                \
/**/

#define ALLSCALE_REGISTER_TREETURE_TYPE(T)                                      \
    ALLSCALE_REGISTER_TREETURE_TYPE_(                                           \
        T,                                                                      \
        BOOST_PP_CAT(BOOST_PP_CAT(treeture, T), _component_type)                \
    )                                                                           \
/**/

#endif

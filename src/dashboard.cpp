
#include <allscale/components/monitor.hpp>
#include <allscale/monitor.hpp>
#include <allscale/dashboard.hpp>
#include <allscale/data_item_manager/get_ownership_json.hpp>
#include <allscale/components/scheduler.hpp>
#include <allscale/scheduler.hpp>
#include <allscale/resilience.hpp>
#ifdef ALLSCALE_HAVE_CPUFREQ
#include <allscale/util/hardware_reconf.hpp>
#endif

#include <hpx/include/actions.hpp>
#include <hpx/lcos/broadcast.hpp>
#include <hpx/async.hpp>
#include <hpx/runtime_fwd.hpp>
#include <hpx/runtime/serialization/serialize.hpp>
#include <hpx/runtime/serialization/string.hpp>
#include <hpx/util/io_service_pool.hpp>
#include <hpx/util/asio_util.hpp>
#include <hpx/util/yield_while.hpp>

#include <boost/asio.hpp>


// VV: Define this to use time/energy/resources instead of speed/energy/efficiency
#define ALTERNATIVE_SCORE 

namespace allscale { namespace dashboard
{
    node_state get_state()
    {
        node_state state;
//
        state.rank = hpx::get_locality_id();
        state.online = true;
        state.active = scheduler::active();

        allscale::components::monitor *monitor_c = &allscale::monitor::get();

        // FIXME: add proper metrics here...
        state.num_cores = monitor_c->get_num_cpus();
        state.cpu_load = monitor_c->get_cpu_load();

        state.ownership = data_item_manager::get_ownership_json();

        state.total_memory = monitor_c->get_node_total_memory();
        state.memory_load = monitor_c->get_consumed_memory();
        state.task_throughput = monitor_c->get_throughput();
        state.weighted_task_throughput = monitor_c->get_weighted_throughput();
        state.idle_rate = monitor_c->get_idle_rate();

        state.network_in = monitor_c->get_network_in();
        state.network_out = monitor_c->get_network_out();

        state.cur_frequency = monitor_c->get_current_freq(0);
        state.max_frequency = monitor_c->get_max_freq(0);

        std::size_t active_cores = scheduler::get().get_active_threads();
        state.last_local_score = scheduler::get().get_last_objective_score();
        state.productive_cycles_per_second = float(state.cur_frequency) * (1.f - state.idle_rate);  // freq to Hz

#if defined(ALTERNATIVE_SCORE)
        state.speed = monitor_c->get_avg_time_last_iterations(100);
        state.efficiency = active_cores;
#else
        state.speed = 1.f - state.idle_rate;
        state.efficiency = state.speed * (float(state.cur_frequency * active_cores) / float(state.max_frequency * state.num_cores));
#endif

#if defined(POWER_ESTIMATE) || defined(ALLSCALE_HAVE_CPUFREQ)
        state.cur_power = monitor_c->get_current_power();
        state.max_power = monitor_c->get_max_power();
#else
        state.max_power = 1.0;
        state.cur_power = 1.0;
#endif
        state.power = state.cur_power / state.max_power;

        return state;
    }
}}

HPX_PLAIN_DIRECT_ACTION(allscale::dashboard::get_state, allscale_dashboard_get_state_action);

namespace allscale { namespace dashboard
{
    template <typename Archive>
    void node_state::serialize(Archive& ar, unsigned)
    {
        ar & rank;
        ar & time;
        ar & online;
        ar & active;
        ar & num_cores;
        ar & cpu_load;
        ar & max_frequency;
        ar & cur_frequency;
        ar & memory_load;
        ar & total_memory;
        ar & task_throughput;
        ar & weighted_task_throughput;
        ar & idle_rate;
        ar & network_in;
        ar & network_out;
        ar & ownership;
        ar & productive_cycles_per_second;
        ar & cur_power;
        ar & max_power;
        ar & speed;
        ar & efficiency;
        ar & power;
        ar & last_local_score;
    }

    std::string node_state::to_json() const
    {
        std::string res;
        res += "{\"id\":" + std::to_string(rank) + ',';
        res += "\"time\":" + std::to_string(time) + ',';
        res += "\"state\":\"" + (online ? (active ? std::string("active\",") : "standby\",") : "offline\"");
        if (!online) {
            res += '}';
            return res;
        }
        res += "\"num_cores\":" + std::to_string(num_cores) + ',';
        res += "\"cpu_load\":" + std::to_string(cpu_load) + ',';
        res += "\"max_frequency\":" + std::to_string(max_frequency) + ',';
        res += "\"cur_frequency\":" + std::to_string(cur_frequency) + ',';
        res += "\"mem_load\":" + std::to_string(memory_load) + ',';
        res += "\"total_memory\":" + std::to_string(total_memory) + ',';
        res += "\"task_throughput\":" + std::to_string(task_throughput) + ',';
        res += "\"weighted_task_througput\":" + std::to_string(weighted_task_throughput) + ',';
        res += "\"network_in\":" + std::to_string(network_in) + ',';
        res += "\"network_out\":" + std::to_string(network_out) + ',';
        res += "\"idle_rate\":" + std::to_string(idle_rate) + ',';
        res += "\"productive_cycles_per_second\":" + std::to_string(productive_cycles_per_second) + ',';
        res += "\"cur_power\":" + std::to_string(cur_power) + ',';
        res += "\"max_power\":" + std::to_string(max_power) + ',';
        res += "\"speed\":" + std::to_string(speed) + ',';
        res += "\"efficiency\":" + std::to_string(efficiency) + ',';
        res += "\"power\":" + std::to_string(power) + ',';
        res += "\"owned_data\":" + ownership;
        res += '}';
        return res;
    }

    system_state::system_state()
      : policy(scheduler::policy())
    {}

    std::string system_state::to_json() const
    {
        std::string res;
        res += "{\"type\":\"status\",";
        res += "\"time\":" + std::to_string(time) + ',';
        res += "\"speed\":" + std::to_string(speed) + ',';
        res += "\"efficiency\":" + std::to_string(efficiency) + ',';
        res += "\"power\":" + std::to_string(power) + ',';
        res += "\"objective_exponent\": {";
            res += "\"speed\": " + std::to_string(speed_exponent) +  ",";
            res += "\"efficiency\": " + std::to_string(efficiency_exponent) + ",";
            res += "\"power\": " + std::to_string(power_exponent) + "";
        res += "},";
        res += "\"score\":" + std::to_string(score()) + ',';
        res += "\"scheduler\": \"" + policy + "\",";
        res += "\"nodes\":[";
        for (auto & node: nodes)
        {
            res += node.to_json() + ',';
        }
        res.back() = ']';
        res += "}";
        return res;
    }

    float system_state::score() const
    {
#if defined(ALTERNATIVE_SCORE)
        return std::exp(speed * speed_exponent) *
                std::exp(efficiency * efficiency_exponent ) *
                std::exp(power * power_exponent);
#else
        return std::pow(speed, speed_exponent) *
               std::pow(efficiency, efficiency_exponent) *
               std::pow(1 - power, power_exponent);
#endif
    }

    template void node_state::serialize<hpx::serialization::input_archive>(hpx::serialization::input_archive& ar, unsigned);
    template void node_state::serialize<hpx::serialization::output_archive>(hpx::serialization::output_archive& ar, unsigned);

    /**
     * The name of the environment variable checked for the IP address of the dashboard server.
     */
    constexpr const char* ENVVAR_DASHBOARD_IP = "ALLSCALE_DASHBOARD_IP";

    /**
     * The name of the environment variable checked for the IP port number of the dashboard server.
     */
    constexpr const char* ENVVAR_DASHBOARD_PORT = "ALLSCALE_DASHBOARD_PORT";

    /**
     * The default dashboard server IP address.
     */
    constexpr const char* DEFAULT_DASHBOARD_IP = "127.0.0.1";

    /**
     * The default port utilized to connect to the dashboard.
     */
    constexpr std::uint16_t DEFAULT_DASHBOARD_PORT = 1337;

    struct dashboard_client
    {
        dashboard_client(dashboard_client const& other) = delete;
        dashboard_client& operator=(dashboard_client const& other) = delete;
        dashboard_client(dashboard_client&& other) = delete;
        dashboard_client& operator=(dashboard_client&& other) = delete;

        dashboard_client()
          : io_service_(hpx::get_thread_pool("io-pool")->get_io_service())
          , socket_(io_service_)
          , enabled_(false)
        {
            boost::asio::ip::tcp::resolver resolver(io_service_);

            const char* host_env = std::getenv(ENVVAR_DASHBOARD_IP);
            const char* port_env = std::getenv(ENVVAR_DASHBOARD_PORT);
            
            std::string host;
            if (host_env)
            {
                host = host_env;
            }
            else
            {
                host = DEFAULT_DASHBOARD_IP;
            }

            std::uint16_t port;
            if (port_env)
            {
                port = std::atoi(port_env);
            }
            else
            {
                port = DEFAULT_DASHBOARD_PORT;
            }

            std::vector<boost::asio::ip::tcp::endpoint> endpoint{
                hpx::util::resolve_hostname(host, port, io_service_)};

            boost::system::error_code ec;
            boost::asio::connect(socket_, endpoint.begin(), endpoint.end(), ec);

            if (ec)
            {
                std::cerr << "Dashboard could not connect to: " << host << ':' << port << '\n';
                return;
            }

            localities_ = hpx::find_all_localities();
            enabled_ = true;

            std::cerr << "Dashboard connected to: " << host << ':' << port << '\n';
        }

        explicit operator bool() const { return enabled_; }

        hpx::future<std::vector<node_state>> get_system_state()
        {
            // Filter out failed localities (ask resilience component)
            std::vector<hpx::naming::id_type> survivors;
            survivors.reserve(localities_.size());
            std::copy_if(localities_.begin(), localities_.end(), std::back_inserter(survivors),
                    [](hpx::naming::id_type const& t){
                    int rank = hpx::naming::get_locality_id_from_id(t);
                    return resilience::rank_running(rank);
                    });

            return hpx::lcos::broadcast<allscale_dashboard_get_state_action>(survivors);
        }

        struct msg
        {
            msg(system_state const& state)
              : json(state.to_json())
              , msg_size(htobe64(json.length()))
            {}

            std::string json;
            std::size_t msg_size;
        };

        struct command
        {
            command() : size_(0) {}

            std::uint64_t size_;
            std::vector<char> command_;
        };

        template <typename F>
        void write(system_state& state, F f)
        {
            // Fill in time...
            state.time = time++;
            for (auto& node: state.nodes)
            {
                node.time = state.time;
            }

            std::shared_ptr<msg> m = std::make_shared<msg>(state);
            std::array<boost::asio::const_buffer, 2> buffers;

            buffers[0] = boost::asio::buffer(&m->msg_size, sizeof(std::uint64_t));
            buffers[1] = boost::asio::buffer(m->json.data(), m->json.length());

            /*
             std::cout << "Sending -----------------------------------\n";
             std::cout << m->json << '\n';
             std::cout << "Sending done ------------------------------\n";
            */
            boost::asio::async_write(socket_, buffers,
                [f = std::move(f), m](boost::system::error_code ec, std::size_t /*length*/)
                {
                    if (ec)
                    {
                        std::cerr << "Write failed...\n";
                        return;
                    }
                    f();
                }
            );
        }

        void get_commands()
        {
            std::shared_ptr<command> cmd = std::make_shared<command>();

            boost::asio::async_read(socket_, boost::asio::buffer(&cmd->size_, sizeof(std::uint64_t)),
                [cmd, this](boost::system::error_code ec, std::size_t /*length*/) mutable
                {
                    if (ec)
                    {
                        std::cerr << "Read failed...\n";
                        return;
                    }

                    std::cout << "got message: " << cmd->size_ << " " << be64toh(cmd->size_) << '\n';

                    cmd->command_.resize(be64toh(cmd->size_));
                    auto buffer = boost::asio::buffer(cmd->command_);
                    boost::asio::async_read(socket_, buffer,
                        [cmd, this](boost::system::error_code ec, std::size_t /*length*/) mutable
                        {
                            auto cmd_it = std::find(cmd->command_.begin(), cmd->command_.end(), ' ');

                            std::string command(cmd->command_.begin(), cmd_it);
                            std::string payload(cmd_it + 1, cmd->command_.end());

                            hpx::future<void> cmd_fut;

                            if (command == "set_scheduler")
                            {
                                std::cerr << "Setting scheduler policy to " << payload << '\n';
                                cmd_fut = scheduler::set_policy(payload);
                            }
                            else if (command == "toggle_node")
                            {
                                std::size_t locality_id = std::stol(payload);
                                std::cerr << "Toggling locality " << locality_id << '\n';
                                cmd_fut = scheduler::toggle_node(locality_id);
                            }
                            else if (command == "set_speed")
                            {
                                float exp = std::stof(payload);
                                std::cout << "Setting speed to " << exp << '\n';
                                cmd_fut = scheduler::set_speed_exponent(exp);
                            }
                            else if (command == "set_efficiency")
                            {
                                float exp = std::stof(payload);
                                std::cout << "Setting efficiency to " << exp << '\n';
                                cmd_fut = scheduler::set_efficiency_exponent(exp);
                            }
                            else if (command == "set_power")
                            {
                                float exp = std::stof(payload);
                                std::cout << "Setting power to " << exp << '\n';
                                cmd_fut = scheduler::set_power_exponent(exp);
                            }
                            else
                            {
                                cmd_fut = hpx::make_ready_future();
                                std::cout << "got unknown command: " << command << ':' << payload << '\n';
                            }

                            // Read next...
                            cmd_fut.then([this](hpx::future<void>){ get_commands(); });
                        }
                    );
                }
            );
        }

        void shutdown()
        {
            if (!enabled_) return;

            system_state state;

            state.speed = 0.f;
            state.efficiency = 0.f;
            state.power = 0.f;

            state.nodes.reserve(localities_.size());
            for (auto loc: localities_)
            {
                node_state s;
                s.rank = hpx::naming::get_locality_id_from_id(loc);
                s.online = false;

                state.nodes.push_back(s);
            }

            msg m(state);
            std::array<boost::asio::const_buffer, 2> buffers;

            buffers[0] = boost::asio::buffer(&m.msg_size, sizeof(std::uint64_t));
            buffers[1] = boost::asio::buffer(m.json.data(), m.json.length());

            boost::system::error_code ec;
            boost::asio::write(socket_, buffers, ec);

            if (ec)
            {
                std::cerr << "shutdown failed...\n";
            }

            enabled_ = false;
            socket_.close();
        }

        static dashboard_client& get();

        boost::asio::io_service& io_service_;
        boost::asio::ip::tcp::socket socket_;
        std::vector<hpx::id_type> localities_;
        std::uint64_t time = 0;
        bool enabled_;
        double use_gopt, use_lopt;
    };

    dashboard_client& dashboard_client::get()
    {
        static dashboard_client client;
        return client;
    }

    void shutdown()
    {
        dashboard_client::get().shutdown();
    }

    void update()
    {
        dashboard_client& client = dashboard_client::get();

        if (!client) return;

        auto start = std::chrono::steady_clock::now();

        auto f = client.get_system_state().then(
        [&client, start](hpx::future<std::vector<node_state>> nodes)
        {
            system_state state;

            auto next_update =
                [start]()
                {
                    using namespace std::chrono_literals;
                    auto duration = std::chrono::steady_clock::now() - start;
                    if (duration < 1s)
                        std::this_thread::sleep_for(1s - duration);
                    update();
                };

            try
            {
                state.nodes = nodes.get();

                // compute overall speed
                float total_speed = 0.f;
//                 float total_speed = 1.f;
//                 std::vector<float> speeds(state.nodes.size(), 0.0f);
                float total_efficiency = 0.f;

                float max_power = 0.f;
                float cur_power = 0.f;

                for (auto const& cur: state.nodes)
                {
                    if (cur.online && cur.active)
                    {
                        total_speed += cur.speed;
//                         total_speed *= cur.speed;
//                         speeds[cur.rank] = cur.speed;
                        total_efficiency += cur.efficiency;
                        cur_power += cur.cur_power;
                    }

                    max_power += cur.max_power;
                }

                state.speed = total_speed / client.localities_.size();
//                 state.speed = std::pow(total_speed, 1.f/client.localities_.size());
#if defined(ALTERNATIVE_SCORE)
                // VV: This is the number of active threads
                state.efficiency = total_efficiency;
#else
                state.efficiency = total_efficiency / client.localities_.size();
#endif
                state.power = (max_power > 0) ? cur_power/max_power : 0;

                auto exponents = scheduler::get_optimizer_exponents();

                state.speed_exponent = hpx::util::get<0>(exponents);
                state.efficiency_exponent = hpx::util::get<1>(exponents);
                state.power_exponent = hpx::util::get<2>(exponents);

                client.write(state, std::move(next_update));
            }
            catch (...)
            {
                next_update();
            }
        });
    }

    void get_commands()
    {
        dashboard_client& client = dashboard_client::get();

        if (!client) return;
        client.get_commands();
    }
}}

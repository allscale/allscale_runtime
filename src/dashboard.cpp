
#include <allscale/dashboard.hpp>

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

namespace allscale { namespace dashboard
{
    node_state get_state()
    {
        node_state state;

        state.rank = hpx::get_locality_id();
        state.online = true;
        state.active = true;


        // FIXME: add proper metrics here...
        state.cpu_load = 1./(hpx::get_locality_id() + 1);

        state.ownership = "[]";

        return state;
    }
}}

HPX_PLAIN_ACTION(allscale::dashboard::get_state, allscale_dashboard_get_state_action);

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
    }

    std::string node_state::to_json() const
    {
        std::string res;
        res += "{\"id\":" + std::to_string(rank) + ',';
        res += "\"time\":" + std::to_string(time) + ',';
        res += "\"state\":\"" + (online ? (active ? std::string("active\",") : "stand-by\",") : "offline\"");
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

    std::string system_state::to_json() const
    {
        std::string res;
        res += "{\"type\":\"status\",";
        res += "\"time\":" + std::to_string(time) + ',';
        res += "\"speed\":" + std::to_string(speed) + ',';
        res += "\"efficiency\":" + std::to_string(efficiency) + ',';
        res += "\"power\":" + std::to_string(power) + ',';
        res += "\"score\":" + std::to_string(score) + ',';
        res += "\"nodes\":[";
        for (auto & node: nodes)
        {
            res += node.to_json() + ',';
        }
        res.back() = ']';
        res += "}";
        return res;
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
            // FIXME: make resilient: filter out failed localities...
            return hpx::lcos::broadcast<allscale_dashboard_get_state_action>(
                localities_);
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

            std::cout << "Sending -----------------------------------\n";
            std::cout << m->json << '\n';
            std::cout << "Sending done ------------------------------\n";

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

        void shutdown()
        {
            if (!enabled_) return;

            system_state state;

            state.speed = .9f;
            state.efficiency = .5f;
            state.power = .3f;
            state.score = 1.f;

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

    private:
        boost::asio::io_service& io_service_;
        boost::asio::ip::tcp::socket socket_;
        std::vector<hpx::id_type> localities_;
        std::uint64_t time = 0;
        bool enabled_;
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

        client.get_system_state().then(
        [&client](hpx::future<std::vector<node_state>> nodes)
        {
            system_state state;

            state.speed = .9f;
            state.efficiency = .5f;
            state.power = .3f;
            state.score = 1.f;

            state.nodes = nodes.get();

            client.write(state,
            []()
            {
                // Send next update in 40 millseconds for 25 FPS.
                using namespace std::chrono_literals;
                std::this_thread::sleep_for(1s);
                update();
            });
        });
    }
}}

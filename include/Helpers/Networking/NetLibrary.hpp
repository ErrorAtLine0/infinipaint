#pragma once
#include <string>
#include <unordered_map>
#include <rtc/rtc.hpp>
#include <queue>

typedef uint64_t MessageOrder;
class NetClient;
class NetServer;

template <class T> std::weak_ptr<T> make_weak_ptr(std::shared_ptr<T> ptr) { return ptr; }

#define RELIABLE_COMMAND_CHANNEL "reliableCommand"
#define UNRELIABLE_COMMAND_CHANNEL "unreliableCommand"
#define RESOURCE_COMMAND_CHANNEL "resourceCommand"
#define MAIN_NETLIBRARY_CHANNEL "main00003" // NOTE: Change this value whenever netcode is incompatible with the previous version

typedef uint8_t MessageCommandType;

class NetLibrary {
    public:
        static void init();
        static void update();
        static std::string get_random_server_local_id();
        static const std::string& get_global_id();

        static void register_server(std::shared_ptr<NetServer> server);
        static void register_client(std::shared_ptr<NetClient> client);

        static void destroy();

        static constexpr size_t LOCALID_LEN = 5;
        static constexpr size_t GLOBALID_LEN = 20;
    private:
        static constexpr size_t FRAGMENT_MESSAGE_STRIDE = 4096;
        static constexpr size_t MAX_BUFFERED_DATA_PER_CHANNEL = 64000;
        static constexpr std::chrono::seconds TIMEOUT_DURATION = std::chrono::seconds(30);

        static bool is_ordered_channel(std::string_view channelName);
        static MessageOrder calc_order_for_queued_message(std::string_view channelName, MessageOrder& mOrder);

        static void finish_client_registration(std::shared_ptr<NetClient> client);

        static void assign_client_connection_to_server(const std::string& serverLocalID, const std::string& clientLocalID, std::shared_ptr<rtc::PeerConnection> connection);
        static void channel_created_in_client(std::shared_ptr<rtc::DataChannel> channel);

        static std::string attach_message_order(MessageOrder order, const std::string& message);
        static MessageOrder get_message_order(rtc::binary& message);

        struct PeerData {
            std::shared_ptr<rtc::PeerConnection> connection;
            std::shared_ptr<rtc::DataChannel> channel;
            std::queue<std::string> messageQueue;
        };

        static void setup_main_data_channel(std::shared_ptr<PeerData> peer, const std::string& id);
        static std::shared_ptr<PeerData> setup_peer_connection(const rtc::Configuration& config, std::weak_ptr<rtc::WebSocket> wws, std::string id);

        static std::atomic<bool> alreadyInitialized;
        static std::string signalingAddr;
        static std::shared_ptr<rtc::WebSocket> ws;
        static std::unordered_map<std::string, std::shared_ptr<PeerData>> peers;
        static std::string globalID;
        static rtc::Configuration config;

        static std::mutex clientListMutex;
        static std::mutex serverListMutex;
        static std::mutex peerListMutex;
        static std::vector<std::weak_ptr<NetClient>> clients;
        static std::vector<std::weak_ptr<NetServer>> servers;

        friend class NetClient;
        friend class NetServer;

        static std::promise<void> wsPromise;
};

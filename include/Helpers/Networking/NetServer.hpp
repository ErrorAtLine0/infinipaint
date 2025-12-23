#pragma once
#include <memory>
#include <string>
#include <rtc/rtc.hpp>
#include <mutex>
#include <unordered_map>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/cereal.hpp>
#include "ByteStream.hpp"
#include "NetLibrary.hpp"

class NetServer : public std::enable_shared_from_this<NetServer> {
    public:
        struct ClientData;
        typedef std::function<void(std::shared_ptr<ClientData>, cereal::PortableBinaryInputArchive&)> NetServerRecvCallback;
        typedef std::function<void(std::shared_ptr<ClientData>)> NetServerConnectCallback;
        typedef std::function<void(std::shared_ptr<ClientData>)> NetServerDisconnectCallback;
        struct ClientData : public std::enable_shared_from_this<ClientData> {
            std::unordered_map<std::string, std::queue<std::shared_ptr<std::stringstream>>> messages;
            std::unordered_map<std::string, PartialFragmentMessage> pfm;

            MessageOrder nextMessageOrderToSend = 0;
            MessageOrder nextMessageOrderToExpect = 0;

            std::unordered_map<std::string, std::shared_ptr<rtc::DataChannel>> channels;

            std::mutex receivedMessagesMutex;
            struct ReceivedMessage {
                std::string channel;
                rtc::binary data;
            };
            std::queue<ReceivedMessage> receivedMessages;

            struct OutgoingMessage {
                MessageOrder order;
                std::shared_ptr<std::stringstream> ss;
            };
            std::unordered_map<std::string, std::queue<OutgoingMessage>> messageQueues;

            std::array<uint64_t, 2> customID;

            void send_queued_messages(NetServer& server);
            void parse_received_messages(NetServer& server);
            void send_str_as_bytes(std::shared_ptr<rtc::DataChannel> channel, const std::string& str);
            NetLibrary::DownloadProgress get_progress_into_fragmented_message(const std::string& channel) const;

            std::atomic<bool> setToDisconnect = false;
            std::chrono::steady_clock::time_point lastMessageTime;

            friend class NetServer;
            friend class NetClient;
        };

        NetServer(const std::string& serverLocalID);
        ~NetServer();
        void update();
        template <typename... Args> void send_items_to_client(const std::shared_ptr<ClientData>& client, const std::string& channel, Args&&... items) {
            send_string_stream_to_client(client, channel, get_string_stream_from_items(items...));
        }
        template <typename... Args> void send_items_to_all_clients(const std::string& channel, Args&&... items) {
            send_string_stream_to_all_clients(channel, get_string_stream_from_items(items...));
        }
        template <typename... Args> void send_items_to_all_clients_except(const std::shared_ptr<ClientData>& client, const std::string& channel, Args&&... items) {
            send_string_stream_to_all_clients_except(client, channel, get_string_stream_from_items(items...));
        }
        template <typename... Args> void send_items_to_client_if(std::function<bool(const std::shared_ptr<ClientData>&)> clientChecker, const std::string& channel, Args&&... items) {
            send_string_stream_to_client_if(clientChecker, channel, get_string_stream_from_items(items...));
        }
        template <typename... Args> std::shared_ptr<std::stringstream> get_string_stream_from_items(Args&&... items) {
            auto ss(std::make_shared<std::stringstream>(std::ios::binary | std::ios::out));
            {
                cereal::PortableBinaryOutputArchive m(*ss);
                (m(items), ...);
            }
            return ss;
        }

        void send_string_stream_to_client(const std::shared_ptr<ClientData>& client, const std::string& channel, const std::shared_ptr<std::stringstream>& ss);
        void send_string_stream_to_all_clients(const std::string& channel, const std::shared_ptr<std::stringstream>& ss);
        void send_string_stream_to_all_clients_except(const std::shared_ptr<ClientData>& client, const std::string& channel, const std::shared_ptr<std::stringstream>& ss);
        void send_string_stream_to_client_if(std::function<bool(const std::shared_ptr<ClientData>&)> clientChecker, const std::string& channel, const std::shared_ptr<std::stringstream>& ss);

        bool is_disconnected() const;

        void add_recv_callback(MessageCommandType commandID, const NetServerRecvCallback& callback);
        void add_disconnect_callback(const NetServerDisconnectCallback& callback);

        const std::vector<std::shared_ptr<ClientData>>& get_client_list() const;

    private:
        void parse_received_messages();
        void send_queued_messages();
        void client_connected(std::shared_ptr<rtc::PeerConnection> connection, const std::string& clientLocalID);

        std::atomic<bool> isDisconnected = false;

        std::string localID;

        std::vector<std::shared_ptr<ClientData>> clients;

        // Adding an add list, because i dont want to block the entire clients vector with a mutex
        std::mutex clientsToAddMutex;
        std::vector<std::shared_ptr<ClientData>> clientsToAdd;

        std::unordered_map<MessageCommandType, NetServerRecvCallback> recvCallbacks;
        NetServerDisconnectCallback disconnectCallback;

        NetClient* directConnectClient = nullptr;
        std::shared_ptr<ClientData> directConnectClientData;

        friend class NetLibrary;
        friend class NetClient;
};

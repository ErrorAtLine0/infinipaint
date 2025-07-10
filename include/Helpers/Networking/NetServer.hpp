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
            struct PartialFragmentMessage {
                std::string partialFragmentMessage;
                uint64_t partialFragmentMessageLoc = 0;
            };
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

            std::unordered_map<std::string, std::queue<std::shared_ptr<std::stringstream>>> messageQueues;

            uint64_t customID;

            void send_queued_messages(NetServer& server);
            void parse_received_messages(NetServer& server);
            void send_str_as_bytes(std::shared_ptr<rtc::DataChannel> channel, const std::string& str);

            std::atomic<bool> setToDisconnect = false;
            std::chrono::steady_clock::time_point lastMessageTime;

            friend class NetServer;
            friend class NetClient;
        };

        NetServer(const std::string& serverLocalID);
        ~NetServer();
        void update();
        template <typename... Args> void send_items_to_client(std::shared_ptr<ClientData> client, const std::string& channel, Args&&... items) {
            if(!client)
                return;

            auto ss(std::make_shared<std::stringstream>());
            {
                cereal::PortableBinaryOutputArchive m(*ss);
                (m(items), ...);
            }

            std::vector<std::shared_ptr<std::stringstream>> fragmentedMessage = fragment_message(ss->view(), NetLibrary::FRAGMENT_MESSAGE_STRIDE);
            auto& messageQueue = client->messageQueues[channel];
            if(fragmentedMessage.empty())
                messageQueue.emplace(ss);
            else if(channel != UNRELIABLE_COMMAND_CHANNEL) { // Just drop unreliable messages that are fragmented
                for(auto& ss2 : fragmentedMessage)
                    messageQueue.emplace(ss2);
            }
        }
        template <typename... Args> void send_items_to_all_clients(const std::string& channel, Args&&... items) {
            send_items_to_client_if([&](std::shared_ptr<ClientData> c) {
                return true;
            }, channel, items...);
        }
        template <typename... Args> void send_items_to_all_clients_except(std::shared_ptr<ClientData> client, const std::string& channel, Args&&... items) {
            send_items_to_client_if([&](std::shared_ptr<ClientData> c) {
                return c != client;
            }, channel, items...);
        }
        template <typename... Args> void send_items_to_client_if(std::function<bool(std::shared_ptr<ClientData>)> clientChecker, const std::string& channel, Args&&... items) {
            auto ss(std::make_shared<std::stringstream>());
            {
                cereal::PortableBinaryOutputArchive m(*ss);
                (m(items), ...);
            }

            std::vector<std::shared_ptr<std::stringstream>> fragmentedMessage = fragment_message(ss->view(), NetLibrary::FRAGMENT_MESSAGE_STRIDE);

            for(auto& client : clients) {
                if(client && clientChecker(client)) {
                    auto& messageQueue = client->messageQueues[channel];
                    if(fragmentedMessage.empty())
                        messageQueue.emplace(ss);
                    else if(channel != UNRELIABLE_COMMAND_CHANNEL) { // Just drop unreliable messages that are fragmented
                        for(auto& ss2 : fragmentedMessage)
                            messageQueue.emplace(ss2);
                    }
                }
            }
        }

        bool is_disconnected();

        void add_recv_callback(MessageCommandType commandID, const NetServerRecvCallback& callback);
        void add_disconnect_callback(const NetServerDisconnectCallback& callback);
    private:
        void parse_received_messages();
        void send_queued_messages();
        void client_connected(std::shared_ptr<rtc::PeerConnection> connection, const std::string& clientLocalID);

        std::atomic<bool> isDisconnected = false;

        std::string localID;

        std::mutex clientList;

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

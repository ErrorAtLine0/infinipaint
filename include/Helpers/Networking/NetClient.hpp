#pragma once
#include <memory>
#include <string>
#include <rtc/rtc.hpp>
#include <cereal/cereal.hpp>
#include <unordered_map>
#include <functional>
#include <cereal/cereal.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <queue>
#include "ByteStream.hpp"
#include "NetLibrary.hpp"

typedef std::function<void(cereal::PortableBinaryInputArchive&)> NetClientRecvCallback;

class NetClient : public std::enable_shared_from_this<NetClient> {
    public:
        NetClient(const std::string& serverFullID);
        NetClient(NetServer& server);
        template <typename... Args> void send_items_to_server(const std::string& channel, Args&&... items) {
            if(isDisconnected)
                return;
            auto ss(std::make_shared<std::stringstream>());
            {
                cereal::PortableBinaryOutputArchive m(*ss);
                (m(items), ...);
            }
            std::vector<std::shared_ptr<std::stringstream>> fragmentedMessage = fragment_message(ss->view(), NetLibrary::FRAGMENT_MESSAGE_STRIDE);
            auto& messageQueue = messageQueues[channel];
            if(fragmentedMessage.empty())
                messageQueue.emplace(NetLibrary::calc_order_for_queued_message(channel, nextMessageOrderToSend), ss);
            else if(channel != UNRELIABLE_COMMAND_CHANNEL) { // Just drop unreliable messages that are fragmented
                for(auto& ss2 : fragmentedMessage)
                    messageQueue.emplace(NetLibrary::calc_order_for_queued_message(channel, nextMessageOrderToSend), ss2);
            }
        }
        void update();
        void add_recv_callback(MessageCommandType commandID, const NetClientRecvCallback& callback);
        bool is_disconnected();
        std::pair<uint64_t, uint64_t> get_progress_into_fragmented_message(const std::string& channel);

    private:
        std::string localID;
        void init_channel(const std::string& channelName, std::shared_ptr<rtc::DataChannel> channel);
        void parse_received_messages();
        void send_queued_messages();
        void send_str_as_bytes(std::shared_ptr<rtc::DataChannel> channel, const std::string& str);

        struct OutgoingMessage {
            MessageOrder order;
            std::shared_ptr<std::stringstream> ss;
        };
        std::unordered_map<std::string, std::queue<OutgoingMessage>> messageQueues;

        struct ReceiveQueue {
            std::mutex mut;
            struct ReceivedMessage {
                std::string channel;
                rtc::binary data;
            };
            std::queue<ReceivedMessage> messages;
        };
        std::shared_ptr<ReceiveQueue> receiveQueue;

        std::unordered_map<std::string, std::shared_ptr<rtc::DataChannel>> channels;

        std::unordered_map<std::string, PartialFragmentMessage> pfm;

        std::unordered_map<MessageCommandType, NetClientRecvCallback> recvCallbacks;

        NetServer* directConnectServer = nullptr;
        std::atomic<bool> isDisconnected = false;

        MessageOrder nextMessageOrderToSend = 0;
        MessageOrder nextMessageOrderToExpect = 0;

        std::string serverFullID;
        bool fullyRegistered = false;

        std::chrono::steady_clock::time_point lastMessageTime;

        friend class NetLibrary;
        friend class NetServer;
};

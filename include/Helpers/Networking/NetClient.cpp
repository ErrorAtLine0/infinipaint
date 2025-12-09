#include "NetClient.hpp"
#include "NetLibrary.hpp"
#include "NetServer.hpp"
#include <chrono>
#include <Helpers/Logger.hpp>

NetClient::NetClient(const std::string& serverFullID) {
    receiveQueue = std::make_shared<ReceiveQueue>();
    this->serverFullID = serverFullID;
    lastMessageTime = std::chrono::steady_clock::now();
}

NetClient::NetClient(NetServer& server) {
    receiveQueue = std::make_shared<ReceiveQueue>();
    directConnectServer = &server;
    server.directConnectClient = this;
    server.directConnectClientData = std::make_shared<NetServer::ClientData>();
    server.directConnectClientData->lastMessageTime = std::chrono::steady_clock::now();
    server.clients.emplace_back(server.directConnectClientData);
    lastMessageTime = std::chrono::steady_clock::now();
}

void NetClient::init_channel(const std::string& channelName, std::shared_ptr<rtc::DataChannel> channel) {
    if(isDisconnected)
        return;
    auto commandChannelOnMessage = [channelName, rQueueWeak = make_weak_ptr(receiveQueue)](rtc::message_variant data) {
        auto rQueue = rQueueWeak.lock();
        if(rQueue) {
            std::scoped_lock rQueueLock(rQueue->mut);
            rQueue->messages.emplace(channelName, std::get<rtc::binary>(data));
        }
    };
    channel->onMessage(commandChannelOnMessage);
    channel->onClosed([cWeak = make_weak_ptr(shared_from_this())]() {
        auto cLock = cWeak.lock();
        if(cLock)
            cLock->isDisconnected = true;
    });
    channels[channelName] = channel;
    Logger::get().log("INFO", "Client channel setup: " + channelName);
    lastMessageTime = std::chrono::steady_clock::now();
}

void NetClient::update() {
    if(isDisconnected)
        return;
    if(std::chrono::steady_clock::now() - lastMessageTime >= NetLibrary::TIMEOUT_DURATION) {
        Logger::get().log("INFO", "Connection timed out");
        isDisconnected = true;
        return;
    }
    parse_received_messages();
    send_queued_messages();
}

void NetClient::parse_received_messages() {
    std::scoped_lock rQueueLock(receiveQueue->mut);
    while(!receiveQueue->messages.empty()) {
        auto& receivedMessage = receiveQueue->messages.front();

        lastMessageTime = std::chrono::steady_clock::now();

        size_t messageOrderDisplacement = 0;
        if(NetLibrary::is_ordered_channel(receivedMessage.channel)) {
            MessageOrder messageOrder = NetLibrary::get_message_order(receivedMessage.data);
            if(nextMessageOrderToExpect > messageOrder && receivedMessage.channel == UNRELIABLE_COMMAND_CHANNEL) {
                receiveQueue->messages.pop();
                continue;
            }
            nextMessageOrderToExpect = std::max(messageOrder + 1, nextMessageOrderToExpect);
            messageOrderDisplacement = sizeof(MessageOrder);
        }

        ByteMemStream strm((char*)(receivedMessage.data.data() + messageOrderDisplacement), receivedMessage.data.size() - messageOrderDisplacement);
        cereal::PortableBinaryInputArchive inArchive(strm);

        auto& spfm = pfm[receivedMessage.channel];

        MessageCommandType commandID;
        inArchive(commandID);

        if(commandID == 0) {
            decode_fragmented_message(inArchive, spfm, NetLibrary::FRAGMENT_MESSAGE_STRIDE, [&](cereal::PortableBinaryInputArchive& completeArchive) {
                MessageCommandType completeCommandID;
                completeArchive(completeCommandID);
                recvCallbacks[completeCommandID](completeArchive);
            });
        }
        else
            recvCallbacks[commandID](inArchive);

        receiveQueue->messages.pop();
    }
}

void NetClient::send_string_stream_to_server(const std::string& channel, const std::shared_ptr<std::stringstream>& ss) {
    if(isDisconnected)
        return;

    auto& messageQueue = messageQueues[channel];

    if(channel == UNRELIABLE_COMMAND_CHANNEL) {
        if(ss->view().length() <= NetLibrary::MAX_UNRELIABLE_MESSAGE_SIZE) // Drop unreliable messages that are too big
            messageQueue.emplace(NetLibrary::calc_order_for_queued_message(channel, nextMessageOrderToSend), ss);
    }
    else {
        std::vector<std::shared_ptr<std::stringstream>> fragmentedMessage = fragment_message(ss->view(), NetLibrary::FRAGMENT_MESSAGE_STRIDE);
        if(fragmentedMessage.empty())
            messageQueue.emplace(NetLibrary::calc_order_for_queued_message(channel, nextMessageOrderToSend), ss);
        else {
            for(auto& ss2 : fragmentedMessage)
                messageQueue.emplace(NetLibrary::calc_order_for_queued_message(channel, nextMessageOrderToSend), ss2);
        }
    }
}

NetLibrary::DownloadProgress NetClient::get_progress_into_fragmented_message(const std::string& channel) const {
    auto it = pfm.find(channel);
    if(it == pfm.end())
        return {0, 0};
    auto& m = it->second;
    if(m.partialFragmentMessage.empty())
        return {0, 0};
    return {m.partialFragmentMessageLoc, m.partialFragmentMessage.size()};
}

void NetClient::send_queued_messages() {
    for(auto& [channelName, messageQueue] : messageQueues) {
        bool addMessageOrder = NetLibrary::is_ordered_channel(channelName);
        if(directConnectServer) {
            auto& c = directConnectServer->directConnectClientData;
            std::scoped_lock rMessLock(c->receivedMessagesMutex);
            while(!messageQueue.empty()) {
                std::string toInsert;
                if(addMessageOrder)
                    toInsert = NetLibrary::attach_message_order(messageQueue.front().order, messageQueue.front().ss->str());
                else
                    toInsert = messageQueue.front().ss->str();
                rtc::binary b((std::byte*)toInsert.c_str(), (std::byte*)(toInsert.c_str() + toInsert.length()));
                c->receivedMessages.emplace(channelName, b);
                messageQueue.pop();
            }
        }
        else {
            auto& channel = channels[channelName];
            if(channel) {
                if(channel->isOpen()) {
                    while(!messageQueue.empty()) {
                        #ifdef __EMSCRIPTEN__
                            if(channel->bufferedAmount() >= NetLibrary::MAX_BUFFERED_DATA_PER_CHANNEL)
                                break;
                        #endif
                        if(addMessageOrder)
                            send_str_as_bytes(channel, NetLibrary::attach_message_order(messageQueue.front().order, messageQueue.front().ss->str()));
                        else
                            send_str_as_bytes(channel, messageQueue.front().ss->str());
                        messageQueue.pop();
                    }
                    #ifdef __EMSCRIPTEN__
                        if(channel->bufferedAmount() >= NetLibrary::MAX_BUFFERED_DATA_PER_CHANNEL && channelName == UNRELIABLE_COMMAND_CHANNEL)
                            messageQueue = std::queue<OutgoingMessage>();
                    #endif
                }
            }
        }
    }
}

bool NetClient::is_disconnected() const {
    return isDisconnected;
}

void NetClient::send_str_as_bytes(std::shared_ptr<rtc::DataChannel> channel, const std::string& str) {
    channel->send((const std::byte*)str.c_str(), str.size());
}

void NetClient::add_recv_callback(MessageCommandType commandID, const NetClientRecvCallback& callback) {
    recvCallbacks[commandID] = callback;
}

NetClient::~NetClient() {
    if(directConnectServer) {
        directConnectServer->directConnectClient = nullptr;
        std::erase(directConnectServer->clients, directConnectServer->directConnectClientData);
        directConnectServer->directConnectClientData = nullptr;
    }
}

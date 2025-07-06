#include "NetServer.hpp"
#include "NetLibrary.hpp"
#include <chrono>
#include <rtc/peerconnection.hpp>
#include <Helpers/Random.hpp>
#include <Helpers/Logger.hpp>
#include "NetClient.hpp"

NetServer::NetServer(const std::string& serverLocalID) {
    localID = serverLocalID;
    Logger::get().log("INFO", "[NetServer::NetServer] Server has local id: " + serverLocalID);
}

void NetServer::update() {
    if(isDisconnected)
        return;
    std::erase_if(clients, [&](auto& client) {
        bool timedOut = std::chrono::steady_clock::now() - client->lastMessageTime > NetLibrary::TIMEOUT_DURATION;
        if(client->setToDisconnect || timedOut) {
            if(disconnectCallback)
                disconnectCallback(*client);
            if(timedOut)
                Logger::get().log("INFO", "Connection timed out");
            return true;
        }
        client->parse_received_messages(*this);
        client->send_queued_messages(*this);
        return false;
    });

    std::scoped_lock clientAddLock(clientsToAddMutex);
    clients.insert(clients.begin(), clientsToAdd.begin(), clientsToAdd.end());
    clientsToAdd.clear();
}

bool NetServer::is_disconnected() {
    return isDisconnected;
}

void NetServer::client_connected(std::shared_ptr<rtc::PeerConnection> connection, const std::string& clientLocalID) {
    if(isDisconnected)
        return;
    auto newClient = std::make_shared<ClientData>();
    newClient->lastMessageTime = std::chrono::steady_clock::now();

    {
        std::scoped_lock clientAddLock(clientsToAddMutex);
        clientsToAdd.emplace_back(newClient);
    }

    Logger::get().log("INFO", "Client connected and creating channels for: " + clientLocalID);
    newClient->channels.emplace(RELIABLE_COMMAND_CHANNEL, connection->createDataChannel(clientLocalID + RELIABLE_COMMAND_CHANNEL));
    newClient->channels.emplace(RESOURCE_COMMAND_CHANNEL, connection->createDataChannel(clientLocalID + RESOURCE_COMMAND_CHANNEL));
    rtc::DataChannelInit unreliableCommandInit;
    unreliableCommandInit.reliability.maxPacketLifeTime = std::chrono::milliseconds(2000);
    unreliableCommandInit.reliability.unordered = true;
    newClient->channels.emplace(UNRELIABLE_COMMAND_CHANNEL, connection->createDataChannel(clientLocalID + UNRELIABLE_COMMAND_CHANNEL, unreliableCommandInit));

    newClient->channels[RELIABLE_COMMAND_CHANNEL]->onMessage([weakClient = make_weak_ptr(newClient)](rtc::message_variant data) {
        std::shared_ptr<ClientData> clientLock = weakClient.lock();
        if(clientLock) {
            std::scoped_lock recvMessLock(clientLock->receivedMessagesMutex);
            clientLock->receivedMessages.emplace(RELIABLE_COMMAND_CHANNEL, std::get<rtc::binary>(data));
        }
    });
    // If any of the local channels are closed, we can assume the client has disconnected
    newClient->channels[RELIABLE_COMMAND_CHANNEL]->onClosed([weakClient = make_weak_ptr(newClient)]() {
        std::shared_ptr<ClientData> clientLock = weakClient.lock();
        if(clientLock)
            clientLock->setToDisconnect = true;
    });
    newClient->channels[UNRELIABLE_COMMAND_CHANNEL]->onMessage([weakClient = make_weak_ptr(newClient)](rtc::message_variant data) {
        std::shared_ptr<ClientData> clientLock = weakClient.lock();
        if(clientLock) {
            std::scoped_lock recvMessLock(clientLock->receivedMessagesMutex);
            clientLock->receivedMessages.emplace(UNRELIABLE_COMMAND_CHANNEL, std::get<rtc::binary>(data));
        }
    });
    newClient->channels[RESOURCE_COMMAND_CHANNEL]->onMessage([weakClient = make_weak_ptr(newClient)](rtc::message_variant data) {
        std::shared_ptr<ClientData> clientLock = weakClient.lock();
        if(clientLock) {
            std::scoped_lock recvMessLock(clientLock->receivedMessagesMutex);
            clientLock->receivedMessages.emplace(RESOURCE_COMMAND_CHANNEL, std::get<rtc::binary>(data));
        }
    });
}

void NetServer::ClientData::send_queued_messages(NetServer& server) {
    for(auto& [channelName, messageQueue] : messageQueues) {
        bool addMessageOrder = channelName == UNRELIABLE_COMMAND_CHANNEL || channelName == RELIABLE_COMMAND_CHANNEL;
        if(this == server.directConnectClientData.get()) {
            NetClient* c = server.directConnectClient;
            std::scoped_lock recvQueueLock(c->receiveQueue->mut);
            while(!messageQueue.empty()) {
                std::string toInsert;
                if(addMessageOrder)
                    toInsert = NetLibrary::attach_message_order_and_update(nextMessageOrderToSend, messageQueue.front()->str());
                else
                    toInsert = messageQueue.front()->str();
                rtc::binary b((std::byte*)toInsert.c_str(), (std::byte*)(toInsert.c_str() + toInsert.length()));
                c->receiveQueue->messages.emplace(channelName, b);
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
                            send_str_as_bytes(channel, NetLibrary::attach_message_order_and_update(nextMessageOrderToSend, messageQueue.front()->str()));
                        else
                            send_str_as_bytes(channel, messageQueue.front()->str());
                        messageQueue.pop();
                    }
                    #ifdef __EMSCRIPTEN__
                        if(channel->bufferedAmount() >= NetLibrary::MAX_BUFFERED_DATA_PER_CHANNEL && channelName == UNRELIABLE_COMMAND_CHANNEL)
                            messageQueue = std::queue<std::shared_ptr<std::stringstream>>();
                    #endif
                }
            }
        }
    }
}

void NetServer::ClientData::send_str_as_bytes(std::shared_ptr<rtc::DataChannel> channel, const std::string& str) {
    channel->send((const std::byte*)str.c_str(), str.size());
}

void NetServer::ClientData::parse_received_messages(NetServer& server) {
    std::scoped_lock recvMessLock(receivedMessagesMutex);
    while(!receivedMessages.empty()) {
        auto& receivedMessage = receivedMessages.front();
        lastMessageTime = std::chrono::steady_clock::now();

        size_t messageOrderDisplacement = 0;
        if(receivedMessage.channel == UNRELIABLE_COMMAND_CHANNEL || receivedMessage.channel == RELIABLE_COMMAND_CHANNEL) {
            MessageOrder messageOrder = NetLibrary::get_message_order(receivedMessage.data);
            if(nextMessageOrderToExpect > messageOrder && receivedMessage.channel == UNRELIABLE_COMMAND_CHANNEL) {
                receivedMessages.pop();
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
            if(spfm.partialFragmentMessage.size() == 0) {
                spfm.partialFragmentMessageLoc = 0;
                uint64_t messageSize;
                inArchive(messageSize);
                spfm.partialFragmentMessage.resize(messageSize);
                inArchive(cereal::binary_data(spfm.partialFragmentMessage.data() + spfm.partialFragmentMessageLoc, NetLibrary::FRAGMENT_MESSAGE_STRIDE));
                spfm.partialFragmentMessageLoc += NetLibrary::FRAGMENT_MESSAGE_STRIDE;
            }
            else if(spfm.partialFragmentMessage.size() - spfm.partialFragmentMessageLoc <= NetLibrary::FRAGMENT_MESSAGE_STRIDE) {
                inArchive(cereal::binary_data(spfm.partialFragmentMessage.data() + spfm.partialFragmentMessageLoc, spfm.partialFragmentMessage.size() - spfm.partialFragmentMessageLoc));
                ByteMemStream completeStrm(spfm.partialFragmentMessage.data(), spfm.partialFragmentMessage.size());
                cereal::PortableBinaryInputArchive completeArchive(completeStrm);
                MessageCommandType completeCommandID;
                completeArchive(completeCommandID);
                server.recvCallbacks[completeCommandID](*this, completeArchive);
                spfm.partialFragmentMessage.clear();
                spfm.partialFragmentMessageLoc = 0;
            }
            else {
                inArchive(cereal::binary_data(spfm.partialFragmentMessage.data() + spfm.partialFragmentMessageLoc, NetLibrary::FRAGMENT_MESSAGE_STRIDE));
                spfm.partialFragmentMessageLoc += NetLibrary::FRAGMENT_MESSAGE_STRIDE;
            }
        }
        else
            server.recvCallbacks[commandID](*this, inArchive);

        receivedMessages.pop();
    }
}

void NetServer::add_recv_callback(MessageCommandType commandID, const NetServerRecvCallback& callback) {
    recvCallbacks[commandID] = callback;
}

void NetServer::add_disconnect_callback(const NetServerDisconnectCallback& callback) {
    disconnectCallback = callback;
}

NetServer::~NetServer() {
    if(directConnectClient)
        directConnectClient->isDisconnected = true;
}

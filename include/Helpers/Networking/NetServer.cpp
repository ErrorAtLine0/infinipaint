#include "NetServer.hpp"
#include "Helpers/Networking/ByteStream.hpp"
#include "NetLibrary.hpp"
#include <chrono>
#include <rtc/peerconnection.hpp>
#include <Helpers/Random.hpp>
#include <Helpers/Logger.hpp>
#include "NetClient.hpp"
#include "cereal/archives/portable_binary.hpp"

NetServer::NetServer(const std::string& serverLocalID) {
    localID = serverLocalID;
    Logger::get().log("INFO", "[NetServer::NetServer] Server has local id: " + serverLocalID);
}

void NetServer::update() {
    if(isDisconnected)
        return;
    for(auto& client : clients) {
        if(client) {
            bool timedOut = std::chrono::steady_clock::now() - client->lastMessageTime > NetLibrary::TIMEOUT_DURATION;
            if(timedOut) {
                client->setToDisconnect = true;
                Logger::get().log("INFO", "Connection timed out");
            }
            if(client->setToDisconnect)
                disconnectCallback(client);
            else {
                #ifdef NDEBUG
                try {
                #endif
                    client->parse_received_messages(*this);
                    client->send_queued_messages(*this);
                #ifdef NDEBUG
                }
                catch(const std::exception& e) {
                    Logger::get().log("INFO", "[NetServer::update] Exception thrown while parsing and sending messages for a client: " + std::string(e.what()));
                    client->setToDisconnect = true;
                    disconnectCallback(client);
                }
                catch(...) {
                    Logger::get().log("INFO", "[NetServer::update] Unknown exception thrown while parsing and sending messages for a client.");
                    client->setToDisconnect = true;
                    disconnectCallback(client);
                }
                #endif
            }
        }
    }

    std::erase_if(clients, [&](auto& client) {
        return !client || client->setToDisconnect.load();
    });

    std::scoped_lock clientAddLock(clientsToAddMutex);
    clients.insert(clients.begin(), clientsToAdd.begin(), clientsToAdd.end());
    clientsToAdd.clear();
}

bool NetServer::is_disconnected() const {
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

void NetServer::send_string_stream_to_client(const std::shared_ptr<ClientData>& client, const std::string& channel, const std::shared_ptr<std::stringstream>& ss) {
    if(!client)
        return;

    auto& messageQueue = client->messageQueues[channel];
    if(channel == UNRELIABLE_COMMAND_CHANNEL) {
        if(ss->view().length() <= NetLibrary::MAX_UNRELIABLE_MESSAGE_SIZE) // Drop unreliable messages that are too big
            messageQueue.emplace(NetLibrary::calc_order_for_queued_message(channel, client->nextMessageOrderToSend), ss);
    }
    else {
        std::vector<std::shared_ptr<std::stringstream>> fragmentedMessage = fragment_message(ss->view(), NetLibrary::FRAGMENT_MESSAGE_STRIDE);
        if(fragmentedMessage.empty())
            messageQueue.emplace(NetLibrary::calc_order_for_queued_message(channel, client->nextMessageOrderToSend), ss);
        else {
            for(auto& ss2 : fragmentedMessage)
                messageQueue.emplace(NetLibrary::calc_order_for_queued_message(channel, client->nextMessageOrderToSend), ss2);
        }
    }
}

void NetServer::send_string_stream_to_all_clients(const std::string& channel, const std::shared_ptr<std::stringstream>& ss) {
    send_string_stream_to_client_if([&](const std::shared_ptr<ClientData>& c) {
        return true;
    }, channel, ss);
}

void NetServer::send_string_stream_to_all_clients_except(const std::shared_ptr<ClientData>& client, const std::string& channel, const std::shared_ptr<std::stringstream>& ss) {
    send_string_stream_to_client_if([&](const std::shared_ptr<ClientData>& c) {
        return c != client;
    }, channel, ss);
}

void NetServer::send_string_stream_to_client_if(std::function<bool(const std::shared_ptr<ClientData>&)> clientChecker, const std::string& channel, const std::shared_ptr<std::stringstream>& ss) {
    if(channel == UNRELIABLE_COMMAND_CHANNEL) {
        if(ss->view().length() <= NetLibrary::MAX_UNRELIABLE_MESSAGE_SIZE) { // Drop unreliable messages that are too big
            for(auto& client : clients) {
                if(client && clientChecker(client)) {
                    auto& messageQueue = client->messageQueues[channel];
                    messageQueue.emplace(NetLibrary::calc_order_for_queued_message(channel, client->nextMessageOrderToSend), ss);
                }
            }
        }
    }
    else {
        std::vector<std::shared_ptr<std::stringstream>> fragmentedMessage = fragment_message(ss->view(), NetLibrary::FRAGMENT_MESSAGE_STRIDE);

        for(auto& client : clients) {
            if(client && clientChecker(client)) {
                auto& messageQueue = client->messageQueues[channel];
                if(fragmentedMessage.empty())
                    messageQueue.emplace(NetLibrary::calc_order_for_queued_message(channel, client->nextMessageOrderToSend), ss);
                else {
                    for(auto& ss2 : fragmentedMessage)
                        messageQueue.emplace(NetLibrary::calc_order_for_queued_message(channel, client->nextMessageOrderToSend), ss2);
                }
            }
        }
    }
}

void NetServer::ClientData::send_queued_messages(NetServer& server) {
    for(auto& [channelName, messageQueue] : messageQueues) {
        bool addMessageOrder = NetLibrary::is_ordered_channel(channelName);
        if(this == server.directConnectClientData.get()) {
            NetClient* c = server.directConnectClient;
            std::scoped_lock recvQueueLock(c->receiveQueue->mut);
            while(!messageQueue.empty()) {
                std::string toInsert;
                if(addMessageOrder)
                    toInsert = NetLibrary::attach_message_order(messageQueue.front().order, messageQueue.front().ss->str());
                else
                    toInsert = messageQueue.front().ss->str();
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

void NetServer::ClientData::send_str_as_bytes(std::shared_ptr<rtc::DataChannel> channel, const std::string& str) {
    channel->send((const std::byte*)str.c_str(), str.size());
}

void NetServer::ClientData::parse_received_messages(NetServer& server) {
    std::scoped_lock recvMessLock(receivedMessagesMutex);
    while(!receivedMessages.empty()) {
        auto& receivedMessage = receivedMessages.front();
        lastMessageTime = std::chrono::steady_clock::now();

        size_t messageOrderDisplacement = 0;
        if(NetLibrary::is_ordered_channel(receivedMessage.channel)) {
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
            decode_fragmented_message(inArchive, spfm, NetLibrary::FRAGMENT_MESSAGE_STRIDE, [&](cereal::PortableBinaryInputArchive& completeArchive) {
                MessageCommandType completeCommandID;
                completeArchive(completeCommandID);
                server.recvCallbacks[completeCommandID](shared_from_this(), completeArchive);
            });
        }
        else
            server.recvCallbacks[commandID](shared_from_this(), inArchive);

        receivedMessages.pop();
    }
}

NetLibrary::DownloadProgress NetServer::ClientData::get_progress_into_fragmented_message(const std::string& channel) const {
    auto it = pfm.find(channel);
    if(it == pfm.end())
        return {0, 0};
    auto& m = it->second;
    if(m.partialFragmentMessage.empty())
        return {0, 0};
    return {m.partialFragmentMessageLoc, m.partialFragmentMessage.size()};
}

void NetServer::add_recv_callback(MessageCommandType commandID, const NetServerRecvCallback& callback) {
    recvCallbacks[commandID] = callback;
}

void NetServer::add_disconnect_callback(const NetServerDisconnectCallback& callback) {
    disconnectCallback = callback;
}

const std::vector<std::shared_ptr<NetServer::ClientData>>& NetServer::get_client_list() const {
    return clients;
}

NetServer::~NetServer() {
    if(directConnectClient) {
        directConnectClient->isDisconnected = true;
        directConnectClient->directConnectServer = nullptr;
    }
}

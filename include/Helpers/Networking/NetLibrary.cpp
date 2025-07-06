#include "NetLibrary.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
#include <rtc/peerconnection.hpp>
#include "../Logger.hpp"
#include <rtc/rtc.hpp>
#include <variant>
#include <vector>
#include <Helpers/Random.hpp>
#include "NetClient.hpp"
#include "NetServer.hpp"
#include <bit>

std::atomic<bool> NetLibrary::alreadyInitialized = false;
std::string NetLibrary::signalingAddr;
std::shared_ptr<rtc::WebSocket> NetLibrary::ws;
std::unordered_map<std::string, std::shared_ptr<NetLibrary::PeerData>> NetLibrary::peers;
std::string NetLibrary::globalID;
rtc::Configuration NetLibrary::config;
std::mutex NetLibrary::serverListMutex;
std::mutex NetLibrary::clientListMutex;
std::mutex NetLibrary::peerListMutex;
std::vector<std::weak_ptr<NetClient>> NetLibrary::clients;
std::vector<std::weak_ptr<NetServer>> NetLibrary::servers;
std::promise<void> NetLibrary::wsPromise;

void NetLibrary::init() {
    if(alreadyInitialized)
        return;

    using json = nlohmann::json;
    std::ifstream f("data/config/p2p.json");

    std::vector<std::string> stunServerList;
    std::vector<std::string> turnServerList;

    rtc::InitLogger(rtc::LogLevel::Info, [](rtc::LogLevel level, std::string message) {
        Logger::get().log("INFO", "[LibDataChannel Log Level " + std::to_string(static_cast<int>(level)) + "] " + message);
    });

    if(f.is_open()) {
        try {
            json j;
            f >> j;

            j.at("signalingServer").get_to<std::string>(signalingAddr);

            std::vector<std::string> stunList = j.at("stunList").get<std::vector<std::string>>();
            for(std::string& s : stunList) {
                if(s.substr(0, 5).compare("stun:"))
                    s.insert(0, "stun:");
                config.iceServers.emplace_back(s);
            }
        }
        catch(...) {
            throw std::runtime_error("[NetLibrary::init] Invalid config/p2p.json");
        }
    }
    else
        throw std::runtime_error("[NetLibrary::init] Could not open config/p2p.json");

    f.close();


    // Not sure why, but TLS verification fails on mac
    // Should be fixed later, but for now this is fine, as the signaling server doesn't have
    // critical information
#ifdef __APPLE__
    rtc::WebSocket::Configuration wsConfig;
    wsConfig.disableTlsVerification = true;
    ws = std::make_shared<rtc::WebSocket>(wsConfig);
#else
    ws = std::make_shared<rtc::WebSocket>();
#endif

    auto wsFuture = wsPromise.get_future();

    ws->onOpen([]() {
        Logger::get().log("INFO", "Websocket connected, signaling ready");
        wsPromise.set_value();
    });

    ws->onError([](std::string s) {
        Logger::get().log("INFO", "Websocket error: " + s);
        wsPromise.set_exception(std::make_exception_ptr(std::runtime_error(s)));
    });

    ws->onClosed([]() {
        Logger::get().log("INFO", "Websocket closed");
        destroy();
    });

    ws->onMessage([wws = make_weak_ptr(ws)](auto data) {
        if(!std::holds_alternative<std::string>(data))
            return;

        json message = json::parse(std::get<std::string>(data));

        auto itID = message.find("id");
        if(itID == message.end())
            return;
        auto id = itID->get<std::string>();

        auto itType = message.find("type");
        auto type = itType->get<std::string>();

        std::shared_ptr<rtc::PeerConnection> peerConnection;
        {
            std::scoped_lock peerListLock(peerListMutex);
            auto peerIt = peers.find(id);
            if(peerIt != peers.end())
                peerConnection = peerIt->second->connection;
            else if(type == "offer") {
                Logger::get().log("INFO", "Websocket answering to " + id);
                auto peer = setup_peer_connection(config, wws, id);
                peers[id] = peer;
                peerConnection = peer->connection;
            }
        }

        if(!peerConnection)
            return;

        if(type == "offer" || type == "answer") {
            auto sdp = message["description"].get<std::string>();
            peerConnection->setRemoteDescription(rtc::Description(sdp, type));
        }
        else if(type == "candidate") {
            auto sdp = message["candidate"].get<std::string>();
            auto mid = message["mid"].get<std::string>();
            peerConnection->addRemoteCandidate(rtc::Candidate(sdp, mid));
        }
    });

    get_global_id();

    Logger::get().log("INFO", "NetLibrary global id: " + globalID);

    const std::string wsPrefix = signalingAddr.find("://") == std::string::npos ? "ws://" : "";
    const std::string url = wsPrefix + signalingAddr + "/" + globalID;

    Logger::get().log("INFO", "Websocket URL is: " + url);
    ws->open(url);

    Logger::get().log("INFO", "NetLibrary waiting for signaling to be connected...");
    //wsFuture.get();

    alreadyInitialized = true;
}

std::string NetLibrary::get_random_server_local_id() {
    return Random::get().alphanumeric_str(LOCALID_LEN);
}

const std::string& NetLibrary::get_global_id() {
    if(globalID.empty())
        globalID = Random::get().alphanumeric_str(GLOBALID_LEN);
    return globalID;
}

void NetLibrary::register_server(std::shared_ptr<NetServer> server) {
    std::scoped_lock serverListLock(serverListMutex);
    servers.emplace_back(server);
}

void NetLibrary::register_client(std::shared_ptr<NetClient> client) {
    client->localID = Random::get().alphanumeric_str(LOCALID_LEN);
    std::scoped_lock serverListLock(clientListMutex);
    clients.emplace_back(client);
}

void NetLibrary::finish_client_registration(std::shared_ptr<NetClient> client) {
    try {
        std::string serverGlobalID = client->serverFullID.substr(0, GLOBALID_LEN);
        std::string serverLocalID = client->serverFullID.substr(GLOBALID_LEN);
        std::scoped_lock peerListLock(peerListMutex);
        if(!peers.contains(serverGlobalID)) {
            auto peer = setup_peer_connection(config, ws, serverGlobalID);
            peers[serverGlobalID] = peer;
            peer->channel = peer->connection->createDataChannel(MAIN_NETLIBRARY_CHANNEL);
            setup_main_data_channel(peer, serverGlobalID);
        }
        peers[serverGlobalID]->messageQueue.emplace(client->localID + serverLocalID);
        client->fullyRegistered = true;
    }
    catch(const std::exception& e) {
        Logger::get().log("INFO", "Invalid client registration: " + std::string(e.what()));
        client->isDisconnected = true;
    }
}

void NetLibrary::update() {
    {
        std::scoped_lock clientListLock(clientListMutex);
        std::erase_if(clients, [](auto& cWeak) {
            auto cLock = cWeak.lock();
            if(!cLock)
                return true;
            if(!cLock->fullyRegistered && ws->isOpen())
                finish_client_registration(cLock);
            return false;
        });
    }

    {
        std::scoped_lock serverListLock(serverListMutex);
        std::erase_if(servers, [](auto& sWeak) {
            return !sWeak.lock();
        });
    }

    {
        std::scoped_lock peerListLock(peerListMutex);
        std::erase_if(peers, [](auto& a) {
            auto& p = a.second;
            rtc::PeerConnection::State s = p->connection->state();
            if(s == rtc::PeerConnection::State::Disconnected || s == rtc::PeerConnection::State::Closed || s == rtc::PeerConnection::State::Failed)
                return true;
            if(p->channel) {
                if(p->channel->isOpen()) {
                    while(!p->messageQueue.empty()) {
                        p->channel->send(p->messageQueue.front());
                        Logger::get().log("INFO", "Sent main channel message: " + p->messageQueue.front());
                        p->messageQueue.pop();
                    }
                }
            }
            return false;
        });
    }
}

std::shared_ptr<NetLibrary::PeerData> NetLibrary::setup_peer_connection(const rtc::Configuration& config, std::weak_ptr<rtc::WebSocket> wws, std::string id) {
    using json = nlohmann::json;
    auto peer = std::make_shared<PeerData>();
    peer->connection = std::make_shared<rtc::PeerConnection>(config);

	peer->connection->onLocalDescription([wws, id](rtc::Description description) {
		json message = {{"id", id},
		                {"type", description.typeString()},
		                {"description", std::string(description)}};

		if (auto ws = wws.lock())
			ws->send(message.dump());
	});

	peer->connection->onLocalCandidate([wws, id](rtc::Candidate candidate) {
		json message = {{"id", id},
		                {"type", "candidate"},
		                {"candidate", std::string(candidate)},
		                {"mid", candidate.mid()}};

		if (auto ws = wws.lock())
			ws->send(message.dump());
	});

	peer->connection->onDataChannel([id, peerWeak = make_weak_ptr(peer)](std::shared_ptr<rtc::DataChannel> dc) {
        auto peer = peerWeak.lock();
        if(peer) {
            if(!peer->channel) {
                if(dc->label() == MAIN_NETLIBRARY_CHANNEL) {
                    peer->channel = dc;
                    setup_main_data_channel(peer, id);
                }
                else {
                    Logger::get().log("INFO", "Client connected with incorrect first channel name: " + dc->label() + ". Could be from a different version of the program");
                    dc->send("REFUSE");
                    peer->connection->close();
                }
            }
            else
                channel_created_in_client(dc);
        }
	});

    return peer;
}

void NetLibrary::setup_main_data_channel(std::shared_ptr<PeerData> peer, const std::string& id) {
    peer->channel->onOpen([id]() { 
        Logger::get().log("INFO", "Main data channel has opened on " + id);
    });
    peer->channel->onClosed([id]() {
        Logger::get().log("INFO", "Main data channel has closed on " + id);
    });
    peer->channel->onMessage([id, pConWeak = make_weak_ptr(peer->connection)](auto data) {
		if(!std::holds_alternative<std::string>(data)) { // Main channel messages will only carry text
            Logger::get().log("INFO", "Main data channel received binary message, which is not expected");
            return;
        }
        std::string message = std::get<std::string>(data);
        if(message == "REFUSE") {
            Logger::get().log("USERINFO", "Refused connection. Could be on a different version");
            auto pConLock = pConWeak.lock();
            if(pConLock)
                pConLock->close();
        }
        else {
            std::string clientLocalID = message.substr(0, LOCALID_LEN);
            std::string serverLocalID = message.substr(LOCALID_LEN);
            Logger::get().log("INFO", "Main data channel received message " + message);
            auto pConLock = pConWeak.lock();
            if(pConLock)
                assign_client_connection_to_server(serverLocalID, clientLocalID, pConLock);
        }
    });
}

void NetLibrary::assign_client_connection_to_server(const std::string& serverLocalID, const std::string& clientLocalID, std::shared_ptr<rtc::PeerConnection> connection) {
    std::scoped_lock serverListLock(serverListMutex);
    auto serverIt = std::find_if(servers.begin(), servers.end(), [&serverLocalID](auto& s) { 
        auto sLock = s.lock();
        if(!sLock)
            return false;
        return sLock->localID == serverLocalID;
    });
    if(serverIt != servers.end()) {
        auto sLock = serverIt->lock();
        if(sLock)
            sLock->client_connected(connection, clientLocalID);
    }
}

void NetLibrary::channel_created_in_client(std::shared_ptr<rtc::DataChannel> channel) {
    std::string clientID = channel->label().substr(0, LOCALID_LEN);
    std::string channelName = channel->label().substr(LOCALID_LEN);
    std::scoped_lock clientListLock(clientListMutex);
    auto clientIt = std::find_if(clients.begin(), clients.end(), [&clientID](auto& c) {
        auto cLock = c.lock();
        if(!cLock)
            return false;
        return cLock->localID == clientID;
    });
    if(clientIt != clients.end()) {
        auto cLock = clientIt->lock();
        if(cLock)
            cLock->init_channel(channelName, channel);
    }
}

std::string NetLibrary::attach_message_order_and_update(MessageOrder& order, const std::string& message) {
    union {
        MessageOrder o;
        char b[sizeof(MessageOrder)];
    } u;
    u.o = (std::endian::native == std::endian::little) ? std::byteswap(order) : order;
    order++;
    return std::string(u.b, sizeof(MessageOrder)) + message;
}

MessageOrder NetLibrary::get_message_order(rtc::binary& message) {
    union {
        MessageOrder o;
        char b[sizeof(MessageOrder)];
    } u;
    std::memcpy(u.b, message.data(), sizeof(MessageOrder));
    if(std::endian::native == std::endian::little)
        u.o = std::byteswap(u.o);
    return u.o;
}

void NetLibrary::destroy() {
    ws = nullptr;
    {
        std::scoped_lock peerListLock(peerListMutex);
        peers.clear();
    }
    {
        std::scoped_lock clientListLock(clientListMutex);
        for(auto& c : clients) {
            auto cLock = c.lock();
            if(cLock)
                cLock->isDisconnected = true;
        }
        clients.clear();
    }
    {
        std::scoped_lock serverListLock(serverListMutex);
        for(auto& s : servers) {
            auto sLock = s.lock();
            if(sLock)
                sLock->isDisconnected = true;
        }
        servers.clear();
    }
    wsPromise = std::promise<void>();
    //globalID.clear(); // Do we need to do this? (Causes problems when global ID changes from the user's side after they copy a lobby address
    alreadyInitialized = false;
}

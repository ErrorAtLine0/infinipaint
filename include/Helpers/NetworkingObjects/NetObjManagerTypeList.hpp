#pragma once
#include "../Networking/NetLibrary.hpp"
#include <cereal/archives/portable_binary.hpp>

namespace NetworkingObjects {

class NetObjManagerTypeList {
    public:
        template <typename T> void register_class(NetTypeIDType netTypeID, std::function<void(T&, cereal::PortableBinaryOutputArchive&)> writeConstructorFunc, std::function<void(T&, cereal::PortableBinaryInputArchive&)> readConstructorFunc, std::function<void(T&, cereal::PortableBinaryInputArchive&)> readUpdateFunc) {
            netTypeIDData.emplace(netTypeID, {
                .readConstructorFunc = [readConstructorFunc](void* obj, cereal::PortableBinaryInputArchive& message) {
                    readConstructorFunc(static_cast<T*>(obj), message);
                },
                .readUpdateFunc = [readUpdateFunc](void* obj, cereal::PortableBinaryInputArchive& message) {
                    readUpdateFunc(static_cast<T*>(obj), message);
                },
                .allocatorFunc = []() { return std::make_shared<T>(); }
            });
            typeIndexData.emplace(std::type_index(typeid(T)), {
                .netTypeID = netTypeID,
                .writeConstructorFunc = [writeConstructorFunc](void* obj, cereal::PortableBinaryInputArchive& message) {
                    writeConstructorFunc(static_cast<T*>(obj), message);
                },
            });
        }
    private:
        friend class NetObjManager;
        template <typename T> friend class NetObjPtr;

        struct TypeIndexContainer {
            NetTypeIDType netTypeID;
            std::function<void(void*, cereal::PortableBinaryOutputArchive&)> writeConstructorFunc;
        };
        std::unordered_map<std::type_index, TypeIndexContainer> typeIndexData;

        struct NetTypeIDContainer {
            std::function<void(void*, cereal::PortableBinaryInputArchive&)> readConstructorFunc;
            std::function<void(void*, cereal::PortableBinaryInputArchive&)> readUpdateFunc;
            std::function<std::shared_ptr<void>()> allocatorFunc;
        };
        std::unordered_map<NetTypeIDType, NetTypeIDContainer> netTypeIDData;
};

}

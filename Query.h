//
// Created by MasterLong on 2023/6/3.
//

#ifndef NETWORK_PROJ_QUERY_H
#define NETWORK_PROJ_QUERY_H

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/asio.hpp>
#include "ConfigManager.h"

namespace flood {
    class Query {
        // ...
    private:
        unsigned long long id;
        std::string file_name;
        int ttl;
        std::string peer_address;
        unsigned short peer_port;
    public:

        // ...
        Query() = default;

        Query(unsigned long long id, std::string file_name, int ttl,
              const std::string &peer_address, unsigned short peer_port)
                : id(id), file_name(std::move(file_name)), ttl(ttl), peer_address(peer_address), peer_port(peer_port) {}

        // getter和setter

        unsigned long long getId() const {
            return id;
        }

        std::string getFileName() const {
            return file_name;
        }

        int getTTL() const {
            return ttl;
        }

        const std::string &getPeerAddress() const {
            return peer_address;
        }

        unsigned short getPeerPort() const {
            return peer_port;
        }

        void setFileName(std::string file_name) {
            this->file_name = std::move(file_name);
        }

        void setTTL(int ttl) {
            this->ttl = ttl;
        }

        void setPeerAddress(const std::string &peer_address) {
            this->peer_address = peer_address;
        }

        void setPeerPort(unsigned short peer_port) {
            this->peer_port = peer_port;
        }

        bool operator==(const Query &rhs) const {
            return id == rhs.id &&
                   file_name == rhs.file_name &&
                   peer_address == rhs.peer_address &&
                   peer_port == rhs.peer_port;
        }

        // 序列化为字节流
        std::string serialize() const {
            std::ostringstream oss;
            boost::archive::text_oarchive oa(oss);
            // 标记为 Query 类型
            oa << MSG_QUERY;
            oa << *this;
            return oss.str();
        }

        // 从字节流反序列化
        static Query deserialize(const std::string &data) {
            std::istringstream iss(data);
            boost::archive::text_iarchive ia(iss);
            Query query;
            ia >> query;
            return query;
        }

    private:
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive &ar, const unsigned int version) {
            ar & id;
            ar & file_name;
            ar & ttl;
            ar & peer_address;
            ar & peer_port;
        }
    };

}

#endif //NETWORK_PROJ_QUERY_H

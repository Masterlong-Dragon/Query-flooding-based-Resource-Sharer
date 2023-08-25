//
// Created by MasterLong on 2023/6/3.
//

#ifndef NETWORK_PROJ_QUERYHIT_H
#define NETWORK_PROJ_QUERYHIT_H

#include <string>
#include <sstream>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/utility.hpp>
#include "ConfigManager.h"

namespace flood {

    class QueryHit {
        // ...
    private:
        unsigned long long id;
        // 以下成员变量是 QueryHit 的核心
        // 用于标识文件的名字
        // 先文件名后文件路径
        std::vector<std::pair<std::string, std::string>> file_name;
        // 用于标识文件的来源
        std::string peer_address;
        unsigned short peer_port;
    public:

        QueryHit() = default;

        QueryHit(unsigned long long id, std::vector<std::pair<std::string, std::string>> file_name,
                 const std::string &peer_address, unsigned short peer_port)
                : id(id), file_name(std::move(file_name)), peer_address(peer_address), peer_port(peer_port) {}

        //getter和setter

        unsigned long long getId() const {
            return id;
        }

        const std::vector<std::pair<std::string, std::string>> getFileName() const {
            return file_name;
        }

        const std::string &getPeerAddress() const {
            return peer_address;
        }

        unsigned short getPeerPort() const {
            return peer_port;
        }

        QueryHit &setFileName(std::vector<std::pair<std::string, std::string>> &file_name) {
            this->file_name = file_name;
            return *this;
        }

        QueryHit &setPeerAddress(const std::string &peer_address) {
            this->peer_address = peer_address;
            return *this;
        }

        QueryHit &setPeerPort(unsigned short peer_port) {
            this->peer_port = peer_port;
            return *this;
        }

        // 序列化为字节流
        std::string serialize() const {
            std::ostringstream oss;
            boost::archive::text_oarchive oa(oss);
            // 标记为 QueryHit 类型
            oa << MSG_QUERY_HIT;
            oa << *this;
            return oss.str();
        }

        // 从字节流反序列化
        static QueryHit deserialize(const std::string &data) {
            std::istringstream iss(data);
            boost::archive::text_iarchive ia(iss);
            QueryHit queryHit;
            ia >> queryHit;
            return queryHit;
        }

    private:
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive &ar, const unsigned int version) {
            ar & id;
            ar & file_name;
            ar & peer_address;
            ar & peer_port;
        }
    };
}

#endif //NETWORK_PROJ_QUERYHIT_H

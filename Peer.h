//
// Created by MasterLong on 2023/6/3.
//

#ifndef NETWORK_PROJ_PEER_H
#define NETWORK_PROJ_PEER_H

#include <filesystem>
#include <unordered_map>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/udp.hpp>

namespace flood {

    class Peer {
    private:
        // peer对应的分享文件夹
        std::filesystem::path root;
        // peer对应的下载文件夹
        std::filesystem::path download;
        // peer对应的ip地址 boost::asio::ip::address
        boost::asio::ip::address address;
        // peer对应的端口号 用于udp广播洪泛
        unsigned short port;
        // peer对应的端口号 用于tcp连接
        unsigned short tcpPort;
        // 维护一个邻居列表
        std::unordered_map<boost::asio::ip::udp::endpoint, bool> neighbors;
    public:
        Peer(std::filesystem::path root = std::filesystem::current_path(),
             std::filesystem::path download = std::filesystem::current_path()) : root(root),
                                                                                 download(download) {}

        Peer(const Peer &peer);

        Peer &setRoot(std::filesystem::path root);

        std::filesystem::path getRoot() const;

        Peer &setDownload(std::filesystem::path download);

        std::filesystem::path getDownload() const;

        Peer &setAddress(boost::asio::ip::address address);

        boost::asio::ip::address getAddress() const;

        Peer &setTCPPort(unsigned short port);

        unsigned short getTCPPort() const;

        Peer &setUDPPort(unsigned short port);

        unsigned short getUDPPort() const;

        Peer &addNeighbor(unsigned short udp_port,
                          boost::asio::ip::address address = boost::asio::ip::address::from_string("127.0.0.1"));

        Peer &removeNeighbor(unsigned short udp_port,
                             boost::asio::ip::address address = boost::asio::ip::address::from_string("127.0.0.1"));

        std::unordered_map<boost::asio::ip::udp::endpoint, bool> getNeighbors();
    };
}

#endif //NETWORK_PROJ_PEER_H

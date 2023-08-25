//
// Created by MasterLong on 2023/6/3.
//

#include "Peer.h"

namespace flood {

    Peer::Peer(const Peer &peer) {
        this->root = peer.root;
        this->download = peer.download;
        this->address = peer.address;
        this->tcpPort = peer.tcpPort;
        this->port = peer.port;
        this->neighbors = peer.neighbors;
    }

    Peer &Peer::setRoot(std::filesystem::path root) {
        this->root = root;
        return *this;
    }

    std::filesystem::path Peer::getRoot() const{
        return root;
    }

    Peer &Peer::setDownload(std::filesystem::path download) {
        this->download = download;
        return *this;
    }

    std::filesystem::path Peer::getDownload() const {
        return download;
    }

    Peer &Peer::setAddress(boost::asio::ip::address address) {
        this->address = address;
        return *this;
    }

    boost::asio::ip::address Peer::getAddress() const{
        return address;
    }

    Peer &Peer::setTCPPort(unsigned short port) {
        this->tcpPort = port;
        return *this;
    }

    unsigned short Peer::getTCPPort() const{
        return tcpPort;
    }

    Peer &Peer::setUDPPort(unsigned short port) {
        this->port = port;
        return *this;
    }

    unsigned short Peer::getUDPPort() const{
        return port;
    }

    Peer &Peer::addNeighbor(unsigned short udp_port, boost::asio::ip::address address) {
        neighbors[boost::asio::ip::udp::endpoint(address, udp_port)] = true;
        return *this;
    }

    std::unordered_map<boost::asio::ip::udp::endpoint, bool> Peer::getNeighbors() {
        return neighbors;
    }

    Peer &Peer::removeNeighbor(unsigned short udp_port, boost::asio::ip::address address) {
        neighbors.erase(boost::asio::ip::udp::endpoint(address, udp_port));
        return *this;
    }
}
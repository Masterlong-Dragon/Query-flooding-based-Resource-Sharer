//
// Created by MasterLong on 2023/6/3.
//

#ifndef NETWORK_PROJ_SERVER_H
#define NETWORK_PROJ_SERVER_H

#include <boost/asio.hpp>
#include <thread>
#include <iostream>
#include <fstream>
#include <string>
#include "Peer.h"
#include "ConfigManager.h"

namespace flood {

    class Server {
    public:
        Server(boost::asio::io_context &ioContext, const Peer &peer,
               MessageCallback message_callback = [](
                       std::string msg, const int tab_index) { std::cout << msg << std::endl; }, std::string _uuid = "")
                : io_context(ioContext),
                  acceptor(ioContext, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), peer.getTCPPort())),
                  message_callback(std::move(message_callback)),
                  serving_count(0),
                  serving_id(0),
                  uuid(std::move(_uuid)) {
            startAccept();
        }

        Server(boost::asio::io_context &ioContext, unsigned short port,
               MessageCallback message_callback = [](
                       std::string msg, const int tab_index) { std::cout << msg << std::endl; }, std::string _uuid = "")
                : io_context(ioContext),
                  acceptor(ioContext, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
                  message_callback(std::move(message_callback)),
                  serving_count(0),
                  serving_id(0),
                  uuid(std::move(_uuid)) {
            startAccept();
        }

        ~Server() = default;

        int getServingCount() const;

    private:
        boost::asio::io_context &io_context;
        boost::asio::ip::tcp::acceptor acceptor;
        // boost::asio::streambuf receive_buffer;
        std::atomic<int> serving_count;
        int serving_id;
        MessageCallback message_callback;
        // uuid
        std::string uuid;

        // 监听tcp连接
        void startAccept();

        void handleClient(std::shared_ptr<boost::asio::ip::tcp::socket> socket);
    };
}

#endif //NETWORK_PROJ_SERVER_H

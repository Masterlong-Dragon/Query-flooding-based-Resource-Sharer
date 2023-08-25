//
// Created by MasterLong on 2023/6/3.
//

#ifndef NETWORK_PROJ_CLIENT_H
#define NETWORK_PROJ_CLIENT_H


#include <boost/asio/ip/udp.hpp>
#include "Query.h"
#include "QueryHit.h"
#include "Peer.h"
#include "DownloadManager.h"

namespace flood {
    class Client {

    private:
        Peer peer;
        std::mutex query_hit_list_mutex;
        // 下载接收处理
        // Query处理部分
        unsigned long long query_id;
        Query last_query;
        Query last_forwarded_query;
        // 因为可能会重复收到QueryHit，所以用unordered_map
        std::unordered_map<boost::asio::ip::tcp::endpoint, QueryHit> query_hit_list;
        // 用于判断是否超时
        bool is_timeout;
        // 定时器用于超时判断
        boost::asio::steady_timer *timeout_timer;
        // UDP部分
        boost::asio::io_context &io_context;
        boost::asio::ip::udp::socket udp_socket;
        boost::asio::ip::udp::endpoint server_endpoint;
        // 下载部分
        DownloadManager download_manager;

        std::atomic<int> query_sent_count;

        // 消息显示回调 代替std::cout
        MessageCallback message_callback;

        // 消息通知回调
        std::function<void(std::unordered_map<boost::asio::ip::tcp::endpoint, QueryHit>)> notify_callback;

    public:
        explicit Client(boost::asio::io_context &io_context, const Peer &peer,
                        MessageCallback message_callback = [](
                                std::string msg, const int tab_index) { std::cout << msg << std::endl; },
                        std::function<void(
                                std::unordered_map<boost::asio::ip::tcp::endpoint, QueryHit>)> notify_callback = [](
                                std::unordered_map<boost::asio::ip::tcp::endpoint, QueryHit> query_hit_list) {},
                        std::string uuid = "")
                : peer(peer),
                  io_context(io_context),
                  udp_socket(io_context, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), peer.getUDPPort())),
                  query_id(0),
                  is_timeout(false),
                  message_callback(std::move(message_callback)),
                  notify_callback(std::move(notify_callback)),
                  download_manager(io_context, peer.getDownload().string(), message_callback, uuid) {
            start();
        }

        ~Client() {
            delete timeout_timer;
        }

        // 启动
        void start();

        // 查询
        Client &inquire(const std::string &file_name);

        // 下载
        Client &download(const QueryHit &query_hit, const int query_hit_index,
                         DownloadManager::ProgressCallback progress_callback,
                         DownloadManager::FinishCallback success_callback);

        // 获取当前下载数目
        int getDownloadNum() const;

        int getQuerySentCount() const;

    private:
        // UDP部分
        void startUDPReceive();

        void handleUDPMessage(const char *message, std::size_t length);

        // Query处理部分
        void processQuery(const Query query);

        void processQueryHit(const QueryHit queryHit);

        void sendQuery(const Query &query);

        void forwardQuery(const Query query);

        void sendQueryHit(const boost::asio::ip::udp::endpoint &endpoint, const QueryHit queryHit);

        void handleTimeout();
    };
}

#endif //NETWORK_PROJ_CLIENT_H

//
// Created by MasterLong on 2023/6/3.
//

#include <iostream>
#include <boost/archive/text_iarchive.hpp>
#include <thread>
#include "Client.h"
#include "fileHelper.h"

namespace flood {

    char buffer[UDP_BUFFER_SIZE];

    void Client::start() {
#ifdef __DEBUG__
        std::cout << "client started." << std::endl;
#endif
        startUDPReceive();
        timeout_timer = new boost::asio::steady_timer(io_context.get_executor());
    }

    void Client::startUDPReceive() {
#ifdef __DEBUG__
        std::cout << "start UDP receive at " << peer.getUDPPort() << std::endl;
#endif
        udp_socket.async_receive_from(
                boost::asio::buffer(buffer),
                server_endpoint,
                [this](boost::system::error_code ec, std::size_t length) {
                    if (!ec) {
                        handleUDPMessage(buffer, length);
                        startUDPReceive();
                    }
                });
    }

    void Client::handleUDPMessage(const char *message, std::size_t length) {
        try {
#ifdef __DEBUG__
            std::cout << "handle UDP message" << std::endl;
#endif
            unsigned int message_type;
            // 反序列化消息
            {
                std::stringstream ss(std::string(message, length));

                boost::archive::text_iarchive ia(ss);
                // 临时变量用于判断消息类型
                ia >> message_type;
            }

            switch (message_type) {
                case MSG_QUERY: {
                    Query query;
                    {
                        std::stringstream ss(std::string(message, length));

                        boost::archive::text_iarchive ia(ss);
                        // 临时变量用于判断消息类型
                        ia >> message_type;
                        ia >> query;
                    }
                    // 异步处理Query
                    boost::asio::post(io_context.get_executor(),
                                      [this, query]() { processQuery(query); });
                    break;
                }
                case MSG_QUERY_HIT: {
                    QueryHit queryHit;
                    {
                        std::stringstream ss(std::string(message, length));

                        boost::archive::text_iarchive ia(ss);
                        // 临时变量用于判断消息类型
                        ia >> message_type;
                        ia >> queryHit;
                    }
                    // 异步处理QueryHit
                    boost::asio::post(io_context.get_executor(),
                                      [this, queryHit]() { processQueryHit(queryHit); });
                    break;
                }
                default: {
#ifdef __DEBUG__
                    std::cerr << "Error: unknown message type." << std::endl;
#endif
                    break;
                }
            }
        } catch (std::exception &e) {
            // 解包失败 丢弃消息
            /*std::cerr << "Error: " << e.what() << std::endl;
            message_callback("Error in UDP: " + std::string(e.what()), ui_tab::TAB_QUERY);*/
        }

    }

    void Client::processQuery(const Query query) {
        // 丢弃自己发的Query
        if (query.getPeerAddress() == peer.getAddress().to_string() && query.getPeerPort() == peer.getUDPPort()) {
            return;
        }
        std::string filename = query.getFileName();
#ifdef __DEBUG__
        std::cout << "process query for: " << filename << std::endl;
#endif
        // 搜索文件
        std::vector<std::pair<std::string, std::string>> files = file::getFilesByPrefix(filename, peer.getRoot());
        // 如果找到了文件
        if (!files.empty()) {
#ifdef __DEBUG__
            std::cout << "find file: " << files[0].first << std::endl;
#endif
            // 构造QueryHit
            QueryHit query_hit(query.getId(), files, peer.getAddress().to_string(), peer.getTCPPort());
            // 发送QueryHit
            sendQueryHit(boost::asio::ip::udp::endpoint(boost::asio::ip::make_address(query.getPeerAddress()),
                                                        query.getPeerPort()), query_hit);
        }
        // 不论如何都继续转发Query
        // 如果TTL > 0
        if (query.getTTL() > 0) {
            // 构造新的Query
            Query new_query(query.getId(), query.getFileName(), query.getTTL() - 1, query.getPeerAddress(),
                            query.getPeerPort());
            // 转发Query
            forwardQuery(new_query);
        }

    }

    void Client::processQueryHit(const QueryHit queryHit) {
#ifdef __DEBUG__
        std::cout << "process query hit for: " << queryHit.getId() << std::endl;
        // 处理其它peer发来的QueryHit
        // 如果QueryHit的ID和上一次查询的ID相同
        std::cout << "last query id: " << last_query.getId() << std::endl;
#endif
        if (queryHit.getId() == last_query.getId() && !is_timeout) {
            // 将QueryHit加入列表
            query_hit_list[boost::asio::ip::tcp::endpoint(
                    boost::asio::ip::make_address(queryHit.getPeerAddress()), queryHit.getPeerPort()
            )] = queryHit;
#ifdef __DEBUG__
            std::cout << "get QueryHit: " << queryHit.getPeerAddress() << " at " << queryHit.getPeerPort() << std::endl;
#endif
        }
    }

    void Client::sendQuery(const Query &query) {
#ifdef __DEBUG__
        std::cout << "send query for: " << query.getFileName() << std::endl;
        // 构造消息
#endif
        std::string serialized = query.serialize();
        // 遍历邻居列表，向每个邻居发送 UDP 数据报
        auto neighbors = peer.getNeighbors();
#ifdef __DEBUG__
        std::cout << "send query to neighbors in total of: " << neighbors.size() << std::endl;
#endif
        for (const auto &neighbor: neighbors) {
            boost::asio::ip::udp::endpoint endpoint = neighbor.first;
#ifdef __DEBUG__
            std::cout << "send query to: " << endpoint.address().to_string() << ":" << endpoint.port() << std::endl;
#endif
            udp_socket.async_send_to(boost::asio::buffer(serialized), endpoint,
                                     [&](const boost::system::error_code &error, std::size_t bytesSent) {
                                         if (error) {
                                             // 处理发送错误
#ifdef __DEBUG__
                                             std::cerr << "Error: " << error.message() << std::endl;
#endif
                                             message_callback("Error in UDP: " + error.message(), ui_tab::TAB_QUERY);
                                         } else {
                                             // 处理发送成功
                                             query_sent_count++;
                                             // 马上进入监听状态
                                             startUDPReceive();
                                         }
                                     });
        }
    }

    void Client::forwardQuery(const Query query) {
        // 如果上次转发的Query和这次的Query相同 则不转发 (除ttl外)
        if (last_query == query)
            return;
        // 记录这次转发的Query
        last_query = query;
        sendQuery(query);
    }


    void Client::sendQueryHit(const boost::asio::ip::udp::endpoint &endpoint, const QueryHit queryHit) {
        // 构造消息
        std::string serialized = queryHit.serialize();
        // 发送消息
        // 保险多发几次
        for (int i = 0; i < 3; i++) {
            // 间隔10ms 保险防止出现奇怪的错误
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            udp_socket.async_send_to(boost::asio::buffer(serialized), endpoint,
                                     [&](const boost::system::error_code &error, std::size_t bytesSent) {
                                         if (error) {
                                             // 处理发送错误
                                             // std::cerr << "Error: " << error.message() << std::endl;
                                             message_callback("Error in UDP: " + error.message(),
                                                              ui_tab::TAB_QUERY);
                                         } else {
                                             // 处理发送成功
#ifdef __DEBUG__
                                             std::cout << "sent query hit: " << std::endl;
#endif
                                         }
                                     });
        }
    }

    void Client::handleTimeout() {
        // 如果查询结果为空
        if (query_hit_list.empty()) {
            // std::cout << "not found." << std::endl;
            message_callback("Timeout. File inquired not found.", ui_tab::TAB_QUERY);
        } else {
            // 打印查询结果
            message_callback("Timeout. Succeeded in inquiring.", ui_tab::TAB_QUERY);
            // 这里应该通过回调通知UI
            notify_callback(query_hit_list);
            /* std::cout << "result: " << query_hit_list.size() << std::endl;
            for (const auto &query_hit: query_hit_list) {
                std::cout << query_hit.first << std::endl;
                for (const auto &file: query_hit.second.getFileName()) {
                    std::cout << "\t" << file.second << std::endl;
                }
            }*/
        }
        timeout_timer->cancel();  // 取消定时器
        is_timeout = true;  // 重置超时标志
    }

    Client &Client::inquire(const std::string &file_name) {
        boost::asio::post(io_context, [=]() {
            // 清空查询结果
            query_hit_list.clear();
            // 构造Query
            Query query(query_id, file_name, QUERY_TTL, peer.getAddress().to_string(), peer.getUDPPort());
            ++query_id;
            // 洪泛
            // 保存最后一次查询
            last_query = query;
            sendQuery(query);
            // 设置超时定时器
            is_timeout = false;
            timeout_timer->expires_after(std::chrono::milliseconds(QUERY_TIMEOUT));  // 设置超时时间
            timeout_timer->async_wait([this](const boost::system::error_code &ec) {
                if (!ec) {
                    handleTimeout();  // 超时回调函数
                }
            });
        });
        return *this;
    }

    Client &Client::download(const QueryHit &query_hit, const int query_hit_index,
                             DownloadManager::ProgressCallback progress_callback,
                             DownloadManager::FinishCallback success_callback) {
        boost::asio::post(io_context, [=]() {
            // 不判断约束条件 到时候有ui层判断
            // 获取QueryHit
            // 获取文件名
            std::string file_name = query_hit.getFileName()[query_hit_index].first;
            // 获取文件路径
            std::string file_path = query_hit.getFileName()[query_hit_index].second;
            // 调用download管理器
            download_manager.addTask(boost::asio::ip::tcp::endpoint(
                                             boost::asio::ip::make_address(query_hit.getPeerAddress()), query_hit.getPeerPort()
                                     ),
                                     file_path, file_name, progress_callback,
                                     success_callback);
        });
        return *this;
    }

    int Client::getDownloadNum() const {
        return download_manager.getTaskCount();
    }

    int Client::getQuerySentCount() const {
        return query_sent_count;
    }

}
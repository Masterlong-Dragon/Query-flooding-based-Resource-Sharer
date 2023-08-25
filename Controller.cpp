//
// Created by MasterLong on 2023/6/4.
//

#include "Controller.h"

namespace flood {
    Controller::Controller() : io_context(BOOST_ASIO_CONCURRENCY_HINT_SAFE),
                               configManager(),
                               client(io_context, configManager.getCurrentPeer(),
                                      [&](std::string msg, const int tab_index) {
                                          UI::addMessage(msg, tab_index);
                                      },
                                      [&](std::unordered_map<boost::asio::ip::tcp::endpoint, QueryHit> query_hit_list) {
                                          UI::QueryCallback(query_hit_list);
                                      }, configManager.getProcessUUID()),
                               server(io_context, configManager.getCurrentPeer(),
                                      [&](std::string msg, const int tab_index) {
                                          UI::addMessage(msg, tab_index);
                                      }, configManager.getProcessUUID()) {

        // 为UI设置回调函数
        UI::setPeerInfoGetter([&]() -> Peer { return configManager.getPeer(); });
        UI::setDownloadCountGetter([&]() -> int { return client.getDownloadNum(); });
        UI::setServingCountGetter([&]() -> int { return server.getServingCount(); });
        UI::setQuerySender([&](std::string query) {
            client.inquire(query);
        });
        UI::setDownloadCallback([&](const QueryHit &query_hit, const int query_hit_index) {
            client.download(query_hit, query_hit_index,
                            [&](int id, double progress) { UI::updateDownloadProgressCallback(id, progress); },
                            [&](int id, bool success) { UI::downloadSuccessCallback(id, success); });
        });

        // cs线程
        cs_thread = std::thread([&]() {
            io_context.run();
        });

        // ui线程
        ui_thread = std::thread([&]() {
            UI::run();
            io_context.stop();
            // 终止其他线程
            cs_thread.detach();
            exit(0);
        });
    }

    void Controller::run() {
        ui_thread.join();
        cs_thread.join();
    }
}
//
// Created by MasterLong on 2023/6/9.
//

#ifndef NETWORK_PROJ_UI_H
#define NETWORK_PROJ_UI_H

#include <gtk/gtk.h>
#include "ConfigManager.h"
#include <boost/asio/ip/tcp.hpp>
#include "QueryHit.h"

namespace flood {
    class UI {
    private:

        // 开始GTK应用
        static void activate(GtkApplication *app, gpointer user_data);

        // 创建主窗口
        static GtkWidget *createQueryPage();

        // 创建下载窗口
        static GtkWidget *createDownloadPage();

        // 创建配置窗口
        static GtkWidget *createConfigPage();

        // 创建状态窗口
        static GtkWidget *createStatusPage();

        static void onQueryButtonClicked(GtkButton *button, gpointer data);

    public:
        UI() = default;

        // 回调函数

        static void addMessage(std::string msg, const int tab_index);

        static void setPeerInfoGetter(std::function<Peer()> getter);

        static void setDownloadCountGetter(std::function<int()> getter);

        static void setServingCountGetter(std::function<int()> getter);

        static void setQuerySender(std::function<void(std::string)> callback);

        static void QueryCallback(std::unordered_map<boost::asio::ip::tcp::endpoint, QueryHit> query_hit_map);

        static void
        setDownloadCallback(std::function<void(const QueryHit &query_hit, const int query_hit_index)> callback);

        static void updateDownloadProgressCallback(const int down_id, const double progress);

        static void downloadSuccessCallback(const int down_id, bool success);

        // 事件主循环
        static void run();
    };
}


#endif //NETWORK_PROJ_UI_H

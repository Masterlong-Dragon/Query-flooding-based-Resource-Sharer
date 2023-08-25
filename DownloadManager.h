//
// Created by MasterLong on 2023/6/4.
//

#ifndef NETWORK_PROJ_DOWNLOADMANAGER_H
#define NETWORK_PROJ_DOWNLOADMANAGER_H


#include <functional>
#include <string>
#include <fstream>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/streambuf.hpp>
#include <utility>
#include <boost/asio/write.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/read.hpp>
#include <future>
#include <deque>
#include <iostream>
#include "QueryHit.h"

namespace flood {

    class DownloadManager {
    public:
        // 定义类型别名
        typedef std::function<void(int download_id, double progress)> ProgressCallback;
        typedef std::function<void(int download_id, bool success)> FinishCallback;

        // 下载任务属性
        struct DownloadTask {
            // 基本属性
            int id{};
            int retry_count{};
            // 网络
            std::shared_ptr<boost::asio::ip::tcp::socket> socket;
            boost::asio::ip::tcp::endpoint server_end;
            // 文件
            std::string file_path;
            std::string file_name;
            size_t file_size;
            // 消息回调
            ProgressCallback progressCallback;
            FinishCallback finishCallback;
            // 文件
            std::array<char, FILE_CHUNK_SIZE> file_buf;
            std::ofstream file_stream;
            size_t progress;
            bool is_file{};

            DownloadTask(int d_id, boost::asio::ip::tcp::endpoint endpoint,
                         std::string fp, std::string fn, ProgressCallback pcb,
                         FinishCallback fcb)
                    : id(d_id), retry_count(0), server_end(std::move(endpoint)), file_path(std::move(fp)),
                      file_name(std::move(fn)),
                      progressCallback(std::move(pcb)),
                      finishCallback(std::move(fcb)), file_stream(), progress(0) {}
        };

        explicit DownloadManager(boost::asio::io_context &ioContext, std::string dir = "./",
                                 MessageCallback message_callback = [](
                                         std::string msg, const int tab_index) { std::cout << msg << std::endl; },
                                 std::string _uuid = "")
                : io_context(ioContext), next_download_id(0), download_dir(std::move(dir)),
                  message_callback(std::move(message_callback)), uuid(std::move(_uuid)) {
        }

        ~DownloadManager();

        // 添加下载任务
        void
        addTask(boost::asio::ip::tcp::endpoint endpoint, const std::string &file_path, const std::string &file_name,
                ProgressCallback progress_callback, FinishCallback finish_callback);

        int getTaskCount() const;


    private:
        boost::asio::io_context &io_context;
        int next_download_id;
        std::list<std::shared_ptr<DownloadTask>> tasks;
        std::string download_dir;
        // 消息显示回调 代替std::cout
        MessageCallback message_callback;
        std::string uuid;

        // 下载任务相关
        int getNextDownloadId();

        // 处理tcp连接
        void handleRead(const boost::system::error_code &error, std::size_t bytes_read, std::size_t file_size,
                        std::shared_ptr<DownloadTask> task);

        // 处理tcp连接 分块读取
        void startRead(std::shared_ptr<DownloadTask> task);

        // 启动下载任务
        void startDownloadTask(std::shared_ptr<DownloadTask> task);

        // 工具 通信验证
        std::size_t getRemoteFileSize(boost::asio::ip::tcp::socket &socket);

        bool getRemoteContentType(boost::asio::ip::tcp::socket &socket);
    };
}

#endif //NETWORK_PROJ_DOWNLOADMANAGER_H

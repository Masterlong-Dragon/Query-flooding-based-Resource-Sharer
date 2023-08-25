//
// Created by MasterLong on 2023/6/4.
//

#include "DownloadManager.h"
#include "fileHelper.h"

namespace flood {

    DownloadManager::~DownloadManager() {
        for (auto &task: tasks) {
            if (task->file_stream.is_open()) {
                task->file_stream.close();
            }
            if (task->socket->is_open()) {
                task->socket->close();
            }
        }
    }

    void DownloadManager::addTask(boost::asio::ip::tcp::endpoint endpoint, const std::string &file_path,
                                  const std::string &file_name,
                                  ProgressCallback progress_callback, FinishCallback finish_callback) {

        int id = getNextDownloadId();

        if (file::existsFileAt(download_dir + '/' + file_name)) {
            message_callback("File already exists.", ui_tab::TAB_DOWNLOAD);
            finish_callback(id, false);
            return;
        }

        auto task = std::make_shared<DownloadTask>(id, endpoint, file_path, file_name, progress_callback,
                                                   finish_callback);
        tasks.emplace_back(task);

        startDownloadTask(task);
    }

    int DownloadManager::getNextDownloadId() {
        return next_download_id++;
    }

    void
    DownloadManager::handleRead(const boost::system::error_code &error, std::size_t bytes_read, std::size_t file_size,
                                std::shared_ptr<DownloadTask> task) {
        if (!error) {
            task->progress += bytes_read;
            double progress = static_cast<double>(task->progress) / file_size;
            task->progressCallback(task->id, progress);

            task->file_stream.write(task->file_buf.data(), bytes_read);

            if (task->progress >= file_size) {
                task->file_stream.close();

                // 下载完成 处理任务完成逻辑
                // 如果是文件夹下载 则解压
                if (!task->is_file)
                    if (!file::extractZipFile(
                            std::filesystem::temp_directory_path().u8string() + "flood_c" + uuid +
                            std::to_string(task->id) +
                            "/" + task->file_name + ".zip", download_dir + "/" + task->file_name))
                        message_callback("Folder extract failed.", ui_tab::TAB_DOWNLOAD);

                task->finishCallback(task->id, true);
                tasks.remove(task);
            } else {
                task->file_buf.fill(0); // 清空缓冲区
                startRead(task);
            }
        } else {
            task->file_stream.close();
            task->finishCallback(task->id, false);
//            std::cerr << "Download failed: ";
//            std::cerr << error.message() << std::endl;
// 没搞明白错误的解决方法
            // 如果是error == boost::asio::error::eof, 说明是socket关闭导致的错误
            // 尝试发起重连
            if (task->retry_count++ < RETRY_COUNT && error == boost::asio::error::eof) {
                startDownloadTask(task);
                return;
            }
            message_callback("Download failed: " + (error == boost::asio::error::eof ?
                                                    "socket closed accidentally" :
                                                    error.message()), ui_tab::TAB_DOWNLOAD);
            tasks.remove(task);
        }
    }

    void DownloadManager::startRead(std::shared_ptr<DownloadTask> task) {
        if (!task->socket->is_open()) {
            // Socket已关闭，处理任务完成逻辑
            task->file_stream.close();
            task->finishCallback(task->id, false);
            tasks.remove(task);
            return;
        }
        boost::asio::async_read(*(task->socket), boost::asio::buffer(task->file_buf),
                                boost::asio::transfer_at_least(1),
                                [this, task](
                                        const boost::system::error_code &error,
                                        std::size_t bytes_read) {
                                    handleRead(error, bytes_read, task->file_size, task);
                                });
    }

    void DownloadManager::startDownloadTask(std::shared_ptr<DownloadTask> task) {
        try {
            // 创建 socket
            task->socket = std::make_shared<boost::asio::ip::tcp::socket>(io_context);
            // 连接到服务器
            task->socket->connect(task->server_end);
            // std::cout << "Connected to " << task->server_end << std::endl;
            // 发送下载请求给服务器
            std::string request = "DOWNLOAD " + task->file_path + '\n';
            boost::asio::write(*(task->socket), boost::asio::buffer(request));
            // std::cout << "Sent request: " << request << std::endl;
            // std::cout << "Open file: " << download_dir + task->file_name << std::endl;
            // 获取文件大小
            std::size_t file_size = getRemoteFileSize(*(task->socket));
            // std::cout << "File size received: " << file_size << std::endl;
            task->file_size = file_size;
            // 获取文件类型
            task->is_file = getRemoteContentType(*(task->socket));
            // 打开本地文件用于写入
            // 是文件夹的话需要加上.zip后缀
            // 下载到临时目录
            std::string full_path = download_dir + '/' + task->file_name;
            if (!task->is_file) {
                std::string temp_dir = std::filesystem::temp_directory_path().u8string() + "flood_c" + uuid +
                                       std::to_string(task->id) + "/";
                std::filesystem::create_directory(temp_dir);
                full_path = temp_dir + task->file_name + ".zip";
            }
            task->file_stream.open(
                    std::filesystem::u8path(full_path),
                    std::ios::binary);
            // 异步读取数据块
            startRead(task);
        } catch (const std::exception &e) {
            // 处理连接和发送请求的异常
            // 调用完成回调函数，通知下载失败
            // std::cerr << e.what() << std::endl;
            message_callback("Error in start downloading: " + std::string(e.what()), ui_tab::TAB_DOWNLOAD);
            // 关闭socket
            task->socket->close();
            task->file_stream.close();
            task->finishCallback(task->id, false);
            tasks.remove(task);
        }
    }

    std::size_t DownloadManager::getRemoteFileSize(boost::asio::ip::tcp::socket &socket) {
        boost::asio::streambuf response_buf;

        // 读取文件大小直到遇到换行符
        boost::asio::read_until(socket, response_buf, '\n');

        std::istream response_stream(&response_buf);
        std::string response;
        std::getline(response_stream, response);
        try {
            // 尝试将字符串转换为数字
            if (response == "ERROR") {
                message_callback("Failed to get file size", ui_tab::TAB_DOWNLOAD);
                throw std::runtime_error("Failed to get file size");
                return 0;
            }
            std::size_t size = std::stoull(response);
            // 解析并返回文件大小
            return size;
        } catch (const std::exception &e) {
            // 处理转换异常
            message_callback("Failed to get file size: " + std::string(e.what()), ui_tab::TAB_DOWNLOAD);
            throw std::runtime_error("Failed to get file size");
        }
        return 0;
    }

    // true是一般文件，false是文件夹
    bool DownloadManager::getRemoteContentType(boost::asio::ip::tcp::socket &socket) {
        boost::asio::streambuf response_buf;

        // 读取文件大小直到遇到换行符
        boost::asio::read_until(socket, response_buf, '\n');

        std::istream response_stream(&response_buf);
        std::string response;
        std::getline(response_stream, response);

        if (response == "ERROR") {
            message_callback("Failed to get file type", ui_tab::TAB_DOWNLOAD);
            message_callback("Server response: " + response, ui_tab::TAB_DOWNLOAD);
            throw std::runtime_error("Failed to get file type");
        }

        if (response == "FILE") { return true; }
        else if (response == "DIR") { return false; }
        else {
            message_callback("Failed to get file type", ui_tab::TAB_DOWNLOAD);
            throw std::runtime_error("Failed to get file type");
        }
    }

    int DownloadManager::getTaskCount() const {
        return next_download_id;
    }
}
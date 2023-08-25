//
// Created by MasterLong on 2023/6/3.
//

#include "Server.h"
#include "fileHelper.h"

namespace flood {
    void Server::startAccept() {
        std::shared_ptr<boost::asio::ip::tcp::socket> socket =
                std::make_shared<boost::asio::ip::tcp::socket>(io_context);

        acceptor.async_accept(*socket, [this, socket](const boost::system::error_code &error) {
            if (!error) {
                // std::cout << "New client connected: " << socket->remote_endpoint() << std::endl;
                handleClient(std::move(socket));
            } else {
                // std::cerr << "Error accepting client: " << error.message() << std::endl;
            }

            startAccept();
        });
    }

    void Server::handleClient(std::shared_ptr<boost::asio::ip::tcp::socket> socket) {
        try {
            boost::asio::streambuf request_buf;
            boost::asio::read_until(*socket, request_buf, '\n');

            std::istream request_stream(&request_buf);
            std::string request;
            std::getline(request_stream, request);

            if (request.substr(0, 8) == "DOWNLOAD") {
                std::string file_path = request.substr(9);

                // 首先检查是不是文件夹
                std::string zip_file_path = "";
                if (std::filesystem::is_directory(std::filesystem::u8path(file_path))) {
                    // 打包文件夹
                    // 创建临时文件夹
                    std::string temp_dir = std::filesystem::temp_directory_path().u8string() + "flood_s" + uuid +
                                           std::to_string(serving_id++);
                    std::filesystem::create_directory(std::filesystem::u8path(temp_dir));
                    // 开始打包文件夹
                    zip_file_path =
                            temp_dir + '/' + std::filesystem::path(file_path).filename().u8string() +
                            ".zip";
                    if (!file::compressFolder(file_path, zip_file_path)) {
#ifdef __DEBUG__
                        std::cout << "Failed to compress folder: " << file_path << std::endl;
#endif
                        boost::asio::write(*socket, boost::asio::buffer("ERROR\n"));
                        socket->close();
                        return;
                    }
                    file_path = zip_file_path;
                }


                std::ifstream file_stream(std::filesystem::u8path(file_path), std::ios::binary);
                if (!file_stream) {
#ifdef __DEBUG__
                    std::cout << "Failed to open file: " << file_path << std::endl;
#endif
                    boost::asio::write(*socket, boost::asio::buffer("ERROR\n"));
                    socket->close();
                    return;
                }

                serving_count++;

                file_stream.seekg(0, std::ios::end);
                std::size_t file_size = file_stream.tellg();
                file_stream.seekg(0, std::ios::beg);

                // 再告诉客户端文件大小
                std::string file_size_str = std::to_string(file_size) + "\n";
                boost::asio::write(*socket, boost::asio::buffer(file_size_str));
                // 再告诉是常规文件还是打包文件夹
                if (zip_file_path.empty()) {
                    boost::asio::write(*socket, boost::asio::buffer("FILE\n"));
                } else {
                    boost::asio::write(*socket, boost::asio::buffer("DIR\n"));
                }

                // std::cout << "Sending file: " << file_path << std::endl;
                std::vector<char> buffer(FILE_CHUNK_SIZE);

                std::size_t total_bytes_sent = 0;
                while (total_bytes_sent < file_size) {
                    std::size_t bytes_to_send = std::min(FILE_CHUNK_SIZE, file_size - total_bytes_sent);
                    file_stream.read(buffer.data(), bytes_to_send);
                    boost::asio::write(*socket, boost::asio::buffer(buffer.data(), bytes_to_send));
                    total_bytes_sent += bytes_to_send;
                }
                // 关闭文件流
                file_stream.close();
                // std::cout << "File sent successfully: " << file_path << std::endl;
            } else {
                // std::cout << "Invalid request: " << request << std::endl;
                // boost::asio::write(*socket, boost::asio::buffer("ERROR\n"));
            }
        } catch (const std::exception &e) {
#ifdef __DEBUG__
            std::cout << "Exception: " << e.what() << std::endl;
#endif
        }
#ifdef __DEBUG__
        std::cout << "Client disconnected: " << socket->remote_endpoint() << std::endl;
#endif
        socket->close();

        serving_count--;
    }

    int Server::getServingCount() const {
        return serving_count;
    }
}
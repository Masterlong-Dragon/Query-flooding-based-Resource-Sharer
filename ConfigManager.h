//
// Created by MasterLong on 2023/6/4.
//

#ifndef NETWORK_PROJ_CONFIGMANAGER_H
#define NETWORK_PROJ_CONFIGMANAGER_H

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include "Peer.h"
#include "fileHelper.h"
#include <iostream>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace flood {

    // 常量
    constexpr unsigned int QUERY_TTL = 10;
    constexpr unsigned int MSG_QUERY = 0;
    constexpr unsigned int MSG_QUERY_HIT = 1;
    constexpr size_t UDP_BUFFER_SIZE = 2048;
    constexpr size_t QUERY_HIT_THRESHOLD = 10;
    constexpr int QUERY_TIMEOUT = 3000;
    constexpr int DOWNLOAD_TIMEOUT = 2000;
    constexpr size_t FILE_CHUNK_SIZE = 1024 * 1024;
    constexpr const char *CONFIG_FILE_PATH = "./config.ini";
    constexpr int RETRY_COUNT = 5;
    enum ui_tab {
        TAB_QUERY = 0,
        TAB_DOWNLOAD = 1,
        TAB_CONFIG = 2
    };

    // 消息回调
    typedef std::function<void(std::string, const int tab_index)> MessageCallback;

    // 配置管理
    class ConfigManager {
    private:
        int peers_cnt;
        Peer currentPeer;
        std::string process_uuid;
    public:
        ConfigManager() {
            // 检查文件和相关目录是否存在
            if (!file::existsFileAt(CONFIG_FILE_PATH)) {
                // windows下用messagebox
#ifdef _WIN32
                MessageBox(NULL, "Config file not found.", "Error", MB_OK);
#else
                std::cerr << "Config file not found." << std::endl;
#endif
                // 创建新文件
                std::ofstream config_file(CONFIG_FILE_PATH);
                config_file << "[Peer]\n"
                               "IP=127.0.0.1\n"
                               "TCPPort=2333\n"
                               "UDPPort=2334\n"
                               "SharedPath=./\n"
                               "DownloadPath =./\n";
                config_file.close();
                exit(1);
            }

            peers_cnt = 0;
            if (!loadConfig(CONFIG_FILE_PATH))
#ifdef _WIN32
                MessageBox(NULL, "Failed to load config file.", "Error", MB_OK);
#else
            std::cerr << "Failed to load config file." << std::endl;
#endif

            // 检查文件夹是否存在
            if (!file::existsFileAt(currentPeer.getRoot())) {
                std::filesystem::create_directory(currentPeer.getRoot());
            }

            if (!file::existsFileAt(currentPeer.getDownload())) {
                std::filesystem::create_directory(currentPeer.getDownload());
            }

            // 生成uuid 方便某些标识工作
            process_uuid = boost::uuids::to_string(boost::uuids::random_generator()());
        }

        std::string getProcessUUID() const {
            return process_uuid;
        }

        int getPeersCnt() const {
            return peers_cnt;
        }

        Peer getPeer() const {
            return currentPeer;
        }

//        std::string getPeerIP() const {
//            return m_ptree.get<std::string>("Peer.IP", "");
//        }
//
//        int getPeerTCPPort() const {
//            return m_ptree.get<int>("Peer.TCPPort", 0);
//        }
//
//        int getPeerUDPPort() const {
//            return m_ptree.get<int>("Peer.UDPPort", 0);
//        }
//
//        std::vector<std::string> getNeighborIPs() const {
//            std::vector<std::string> neighborIPs;
//            for (const auto &section: m_ptree) {
//                if (section.first.find("Neighbor") != std::string::npos) {
//                    std::string ip = section.second.get<std::string>("IP", "");
//                    neighborIPs.push_back(ip);
//                }
//            }
//            return neighborIPs;
//        }
//
//        int getNeighborUDPPort(const std::string &neighborID) const {
//            return m_ptree.get<int>(neighborID + ".UDPPort", 0);
//        }

        Peer getCurrentPeer() {
            Peer peer;

            // Get Peer section
            boost::property_tree::ptree peerSection = m_ptree.get_child("Peer");

            // Get Peer properties
            std::string ip = peerSection.get<std::string>("IP");
            unsigned short tcpPort = peerSection.get<unsigned short>("TCPPort");
            unsigned short udpPort = peerSection.get<unsigned short>("UDPPort");
            std::string downloadPath = peerSection.get<std::string>("DownloadPath");
            std::string sharedPath = peerSection.get<std::string>("SharedPath");

            // Set Peer properties
            peer.setAddress(boost::asio::ip::address::from_string(ip));
            peer.setTCPPort(tcpPort);
            peer.setUDPPort(udpPort);
            peer.setDownload(downloadPath);
            peer.setRoot(sharedPath);

            // Get Neighbors
            for (const auto &neighborSection: m_ptree) {
                std::string sectionName = neighborSection.first;
                if (sectionName != "Peer") {
                    boost::asio::ip::address neighborAddress = boost::asio::ip::address::from_string(
                            neighborSection.second.get<std::string>("IP"));
                    unsigned short neighborUDPPort = neighborSection.second.get<unsigned short>("UDPPort");
                    peer.addNeighbor(neighborUDPPort, neighborAddress);
                    peers_cnt++;
                }
            }

            return currentPeer = peer;
        }

    private:
        boost::property_tree::ptree m_ptree;

        bool loadConfig(const std::string &filename) {
            try {
                boost::property_tree::ini_parser::read_ini(filename, m_ptree);
                return true;
            } catch (const boost::property_tree::ini_parser_error &e) {
#ifdef _WIN32
                MessageBox(NULL, "Failed to load config file.", "Error", MB_OK);
#else
                std::cerr << "Failed to load config file." << std::endl;
#endif
                return false;
            }
        }
    };
}
#endif //NETWORK_PROJ_CONFIGMANAGER_H

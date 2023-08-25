//
// Created by MasterLong on 2023/6/4.
//

#ifndef NETWORK_PROJ_CONTROLLER_H
#define NETWORK_PROJ_CONTROLLER_H

#include "ConfigManager.h"
#include "Client.h"
#include "Server.h"
#include "UI.h"


namespace flood {
    // 控制器
    // 虽然我感觉完全没有按分层领域模型设计(逃
    class Controller {

    public:
        Controller();

        void run();

    private:
        boost::asio::io_context io_context;
        ConfigManager configManager;
        Client client;
        Server server;
        std::thread ui_thread;
        std::thread cs_thread;
    };
}
#endif //NETWORK_PROJ_CONTROLLER_H

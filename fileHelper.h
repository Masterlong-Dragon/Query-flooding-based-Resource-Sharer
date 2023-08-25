//
// Created by MasterLong on 2023/6/3.
//

#ifndef NETWORK_PROJ_FILEHELPER_H
#define NETWORK_PROJ_FILEHELPER_H

#include <vector>
#include <filesystem>
#include <string>
#include <zip.h>

namespace flood {
    namespace file {
        // 获取所有包含指定前缀的文件名
        std::vector<std::pair<std::string, std::string>>
        getFilesByPrefix(const std::string &prefix, const std::filesystem::path root_path);

        // 检查目录下是否存在指定名称的文件
        bool existsFileAt(const std::filesystem::path file_path);

        // 打开目标文件目录
        void showFileInFolder(const std::filesystem::path file_path);

        // 压缩文件夹
        bool compressFolder(const std::string &sourceFolder, const std::string &compressedFile);

        // 解压缩文件到文件夹
        bool extractZipFile(const std::string &zipFile, const std::string &extractDir);
    }
}

#endif //NETWORK_PROJ_FILEHELPER_H

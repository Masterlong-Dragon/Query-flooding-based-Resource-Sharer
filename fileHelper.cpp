//
// Created by MasterLong on 2023/6/10.
//

#include "fileHelper.h"
#include <Windows.h>
#include <thread>
#include <iostream>

#include <string>
#include <zip.h>
#include <sys/stat.h>

#ifdef _WIN32

#include <direct.h>
#include <fstream>

#define mkdir(dir, mode) _mkdir(dir)
#else
#include <sys/types.h>
#endif

namespace flood {
    namespace file {
        std::vector<std::pair<std::string, std::string>>
        getFilesByPrefix(const std::string &prefix, const std::filesystem::path root_path) {
            std::vector<std::pair<std::string, std::string>> files;
            // 遍历root_path及其子目录下的所有文件
            for (const auto &entry: std::filesystem::recursive_directory_iterator(root_path)) {
                // 如果文件名有prefix
                if (entry.path().filename().string().find(prefix) == 0) {
                    // 将文件名加入到files中
                    files.push_back(std::make_pair(entry.path().filename().string(), entry.path().string()));
                }
            }
            return files;
        }

        // 检查目录下是否存在指定名称的文件
        bool existsFileAt(const std::filesystem::path file_path) {
            // 直接检查文件是否存在
            return std::filesystem::exists(file_path);
        }

        // 在文件资源管理器中显示文件
        void showFileInFolder(const std::filesystem::path file_path) {
// 如果在Windows平台
#ifdef _WIN32
            // 使用cmd命令打开文件资源管理器
            // 打开文件资源管理器并显示指定文件所在位置
            // 要打开的文件资源管理器路径
            std::wstring folderPath = absolute(file_path.parent_path()).wstring();
            // 要显示的文件所在位置
            std::wstring selectPath = absolute(file_path).wstring();
            // cmd命令
            std::wstring cmd = L"explorer /e,/select, " + selectPath;
            // 打开文件资源管理器
            // 异步
            ShellExecuteW(NULL, L"open", L"explorer.exe", cmd.c_str(), folderPath.c_str(), SW_SHOW);
#endif

// 如果在Linux平台
#ifdef __linux__
            // 使用xdg-open命令打开文件资源管理器
            // 没试过行不行
            std::string cmd = "xdg-open " + file_path.string();
            system(cmd.c_str());
#endif
        }

        bool compressFolder(const std::string &sourceFolder, const std::string &compressedFile) {
            // 创建一个新的ZIP存档
            zip *archive = zip_open(compressedFile.c_str(), ZIP_CREATE | ZIP_EXCL, nullptr);
            if (!archive) {
#ifdef __DEBUG__
                std::cout << "Failed to create archive." << std::endl;
#endif
                return false;
            }

            // 遍历源文件夹中的所有文件和子文件夹
            for (const auto &entry: std::filesystem::recursive_directory_iterator(sourceFolder)) {
                const std::string &filePath = entry.path().string();
                const std::string &relativePath = filePath.substr(sourceFolder.length() + 1); // 获取相对路径

                if (std::filesystem::is_regular_file(entry)) {
                    // 打开源文件
                    zip_source *source = zip_source_file(archive, filePath.c_str(), 0, -1);
                    if (!source) {
#ifdef __DEBUG__
                        std::cout << "Failed to open file: " << filePath << std::endl;
#endif
                        zip_close(archive);
                        return false;
                    }

                    // 添加文件到ZIP存档中
                    zip_int64_t index = zip_file_add(archive, relativePath.c_str(), source, ZIP_FL_ENC_UTF_8);
                    if (index < 0) {
#ifdef __DEBUG__
                        std::cout << "Failed to add file to archive: " << filePath << std::endl;
#endif
                        zip_source_free(source);
                        zip_close(archive);
                        return false;
                    }
                } else if (std::filesystem::is_directory(entry)) {
                    // 创建文件夹在ZIP存档中
                    zip_int64_t index = zip_dir_add(archive, relativePath.c_str(), ZIP_FL_ENC_UTF_8);
                    if (index < 0) {
#ifdef __DEBUG__
                        std::cout << "Failed to add directory to archive: " << filePath << std::endl;
#endif
                        zip_close(archive);
                        return false;
                    }
                }
            }

            // 关闭ZIP存档
            if (zip_close(archive) != 0) {
#ifdef __DEBUG__
                std::cout << "Failed to close archive." << std::endl;
#endif
                return false;
            }

            return true;
        }

        bool extractZipFile(const std::string &zipFile, const std::string &extractDir) {

            // 如果不存在解压目录，创建解压目录
            if (!existsFileAt(extractDir)) {
                if (!std::filesystem::create_directory(extractDir)) {
#ifdef __DEBUG__
                    std::cerr << "Failed to create directory: " << extractDir << std::endl;
#endif
                    return false;
                }
            }

            // 打开压缩文件
            zip *archive = zip_open(zipFile.c_str(), 0, nullptr);
            if (!archive) {
#ifdef __DEBUG__
                std::cerr << "Failed to open zip file: " << zipFile << std::endl;
#endif
                return false;
            }

            // 获取压缩文件中的文件数量
            int numFiles = zip_get_num_entries(archive, 0);
            if (numFiles < 0) {
#ifdef __DEBUG__
                std::cerr << "Failed to get number of files in zip" << std::endl;
#endif
                zip_close(archive);
                return false;
            }

            // 逐个解压文件
            for (int i = 0; i < numFiles; ++i) {
                // 获取文件信息
                zip_stat_t fileInfo;
                if (zip_stat_index(archive, i, 0, &fileInfo) != 0) {
#ifdef __DEBUG__
                    std::cerr << "Failed to get file info for index " << i << std::endl;
#endif
                    zip_close(archive);
                    return false;
                }

                // 确定解压缩后的文件路径
                std::filesystem::path extractedFilePath = std::filesystem::u8path(extractDir) / fileInfo.name;

                // 检查文件是否为文件夹
                if (fileInfo.name[strlen(fileInfo.name) - 1] == '/') {
                    // 如果是文件夹，则创建对应的文件夹
                    if (!std::filesystem::create_directories(extractedFilePath)) {
                        std::cerr << "Failed to create directory: " << extractedFilePath << std::endl;
                        zip_close(archive);
                        return false;
                    }
                } else {
                    // 如果是文件，则解压文件内容
                    zip_file *file = zip_fopen_index(archive, i, 0);
                    if (!file) {
#ifdef __DEBUG__
                        std::cerr << "Failed to open file at index " << i << std::endl;
#endif
                        zip_close(archive);
                        return false;
                    }

                    // 创建解压后的文件
                    // 如果父文件夹不存在 先创建父文件夹
                    if (!existsFileAt(std::filesystem::u8path(extractedFilePath.string()).parent_path())) {
                        if (!std::filesystem::create_directories(extractedFilePath.parent_path())) {
#ifdef __DEBUG__
                            std::cerr << "Failed to create directory: " << extractedFilePath.parent_path() << std::endl;
#endif
                            zip_fclose(file);
                            zip_close(archive);
                            return false;
                        }
                    }

                    std::ofstream extractedFile(std::filesystem::u8path(extractedFilePath.string()), std::ios::binary);
                    if (!extractedFile) {
#ifdef __DEBUG__
                        std::cerr << "Failed to create file: " << extractedFilePath << std::endl;
#endif
                        zip_fclose(file);
                        zip_close(archive);
                        return false;
                    }

                    // 读取并写入文件内容
                    char buf[1024];
                    zip_int64_t bytesRead;
                    while ((bytesRead = zip_fread(file, buf, sizeof(buf))) > 0) {
                        extractedFile.write(buf, bytesRead);
                    }

                    // 关闭文件
                    extractedFile.close();
                    zip_fclose(file);
                }
            }

            // 关闭压缩文件
            zip_close(archive);

            return true;
        }
    }
}
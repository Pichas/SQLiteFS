#include "sqlitefs/sqlitefs.h"

int main(int /*unused*/, char** /*unused*/) {
    spdlog::set_level(spdlog::level::debug); // Set global log level to debug


    SQLITEFS;

    spdlog::info("{}", SQLITEFS.pwd());

    // SQLITEFS.mkdir("test");
    // SQLITEFS.mkdir("test2");

    {
        auto dirs = SQLITEFS.ls();
        for (auto d : dirs) {
            spdlog::info("{}", d);
        }
    }

    spdlog::info("-----");
    SQLITEFS.cd("test");
    spdlog::info("{}", SQLITEFS.pwd());

    {
        auto dirs = SQLITEFS.ls();
        for (auto d : dirs) {
            spdlog::info("{}", d);
        }
    }


    spdlog::info("-----");
    SQLITEFS.cd("..");
    spdlog::info("{}", SQLITEFS.pwd());

    {
        auto dirs = SQLITEFS.ls();
        for (auto d : dirs) {
            spdlog::info("{}", d);
        }
    }


    spdlog::info("-----");
    SQLITEFS.rm("test2");
    spdlog::info("{}", SQLITEFS.pwd());

    {
        auto dirs = SQLITEFS.ls();
        for (auto d : dirs) {
            spdlog::info("{}", d);
        }
    }
    ///


    // std::ifstream fs(path, std::ios::in | std::ios::binary);
    // if (!fs) {
    //     spdlog::critical("Error opening: {}", path);
    //     continue;
    // }
}
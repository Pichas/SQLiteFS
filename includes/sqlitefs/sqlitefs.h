#pragma once

#include <functional>
#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>


struct SQLiteFSNode final {
    std::uint32_t id        = 0;
    std::uint32_t parent_id = 0;
    std::string   name;
    std::int64_t  size     = 0;
    std::int64_t  size_raw = 0;
    std::string   compression;
    bool          is_file : 1 = false;

    auto operator<=>(const SQLiteFSNode&) const noexcept = default;
};

struct SQLiteFS final {
    using Data            = std::uint8_t;
    using DataInput       = std::span<const Data>;
    using DataOutput      = std::vector<Data>;
    using ConvertFunc     = std::function<DataOutput(DataInput)>;
    using ConvertFuncsMap = std::unordered_map<std::string, ConvertFunc>;

    SQLiteFS(std::string path);
    ~SQLiteFS();

    const std::string& path() const noexcept;

    bool                      mkdir(const std::string& name);
    bool                      cd(const std::string& name);
    bool                      rm(const std::string& name);
    std::string               pwd() const;
    std::vector<SQLiteFSNode> ls(const std::string& path = ".") const;
    bool                      put(const std::string& name, DataInput data, const std::string& alg = "raw");
    DataOutput                get(const std::string& name) const;
    bool                      mv(const std::string& from, const std::string& to);
    bool                      cp(const std::string& from, const std::string& to);

    void registerSaveFunc(const std::string& name, const ConvertFunc& func);
    void registerLoadFunc(const std::string& name, const ConvertFunc& func);

    DataOutput callSaveFunc(const std::string& name, DataInput data);
    DataOutput callLoadFunc(const std::string& name, DataInput data);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
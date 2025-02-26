#pragma once

#include <functional>
#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace SQLite // NOLINT
{
class Database; // forward decl for raw call
} // namespace SQLite

struct SQLiteFSNode final {
    std::uint32_t id        = 0;
    std::uint32_t parent_id = 0;
    std::string   name;
    std::int64_t  size     = 0;
    std::int64_t  size_raw = 0;
    std::string   compression;

    using Attributes      = enum : std::uint32_t { FILE = 1 };
    Attributes attributes = {};

    auto operator<=>(const SQLiteFSNode&) const noexcept = default;
};

struct SQLiteFS {
    using Data            = char;
    using DataInput       = std::span<const Data>;
    using DataOutput      = std::vector<Data>;
    using ConvertFunc     = std::function<DataOutput(DataInput)>;
    using ConvertFuncsMap = std::unordered_map<std::string, ConvertFunc>;

    SQLiteFS(std::string path, std::string_view key = "");
    virtual ~SQLiteFS();

    const std::string& path() const noexcept;
    void               vacuum();
    std::string        error() const;

    bool                      mkdir(const std::string& name);
    bool                      cd(const std::string& name);
    bool                      rm(const std::string& name);
    std::string               pwd() const;
    std::vector<SQLiteFSNode> ls(const std::string& path = ".") const;
    bool                      write(const std::string& name, DataInput data, const std::string& alg = "raw");
    DataOutput                read(const std::string& name) const;
    bool                      mv(const std::string& from, const std::string& to);
    bool                      cp(const std::string& from, const std::string& to);

    void registerSaveFunc(const std::string& name, const ConvertFunc& func);
    void registerLoadFunc(const std::string& name, const ConvertFunc& func);

    DataOutput callSaveFunc(const std::string& name, DataInput data);
    DataOutput callLoadFunc(const std::string& name, DataInput data);

protected:
    // if you want to expand interface
    void rawCall(const std::function<void(SQLite::Database*)>& callback);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

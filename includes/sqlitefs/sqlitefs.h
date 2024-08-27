#pragma once

#include <memory>
#include <span>
#include <string>
#include <vector>


struct SQLiteFS final {
    SQLiteFS(std::string path);
    ~SQLiteFS();

    const std::string& path() const noexcept;

    bool                      mkdir(const std::string& name);
    bool                      cd(const std::string& name);
    bool                      rm(const std::string& name);
    std::string               pwd() const;
    std::vector<std::string>  ls() const;
    bool                      put(const std::string& name, std::span<const std::uint8_t> data);
    std::vector<std::uint8_t> get(const std::string& name) const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
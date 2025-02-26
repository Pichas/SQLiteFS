# SQLiteFS

Simulate a local fs inside a sqlite database. Based on SQLite 3.49.1.

Uses:

* [SQLite3MultipleCiphers](https://github.com/utelle/SQLite3MultipleCiphers.git) to encrypt the whole database file including headers
* [SQLiteCpp](https://github.com/SRombauts/SQLiteCpp.git) as sqlite wrapper
* [minizip](https://github.com/zlib-ng/minizip-ng.git) for data compression (optional)
* [Tracy](https://github.com/wolfpld/tracy.git) if you need a profiler (optional)

## Support data modifications

You can set a callback to modify the data before storing the data in the DB, such as compression or encryption, and another callback to decompress or decrypt. You can also combine them (zip+aes).

You can also derive from SQLiteFS type and expand interface by using the `void rawCall(const std::function<void(SQLite::Database*)>& callback);` function from derived type.

## Supported functions

* pwd - print working directory
* mkdir - create a new folder
* cd - change wirking directory
* ls - list files and folders
* cp - copy file. For now you can only copy file by file. Folders isn't supported
* mv - move node (file or folder)
* rm - remove node
* write - write file to the db
* read - read file from the db

>NOTE: all operations are thread safe

### Example

```cpp
    std::string               data("random test data");
    std::vector<std::uint8_t> content{data.begin(), data.end()};

    SQLiteFS fs("./fs.db"); // without encryption
    // SQLiteFS fs("./fs.db", "key"); to use encryption
    fs.mkdir("folder");
    fs.cd("folder");

    fs.write("test.txt", content, "raw");
    // fs.write("test.txt", content, "lzma"); // if compression enabled
    auto read_data = fs.read("test.txt");

    ASSERT_EQ(read_data, content);
```

You also can create your own way to store data.

```cpp
    fs->registerSaveFunc("reverse",
                         [](SQLiteFS::DataInput data) { 
                            return SQLiteFS::DataOutput{data.rbegin(), data.rend()}; });

    fs->registerLoadFunc("reverse",
                         [](SQLiteFS::DataInput data) { 
                            return SQLiteFS::DataOutput{data.rbegin(), data.rend()}; });


    std::string               data("random test data");
    std::vector<std::uint8_t> content{data.begin(), data.end()};

    fs->write("test.txt", content, "reverse");
    auto read_data = fs->read("test.txt");

    ASSERT_EQ(read_data.size(), content.size());
    ASSERT_EQ(read_data, content);
```

Check tests for more examples

## How to include into your project

### CMakeLists Example

```cmake
project(MyCoolProject)
add_executable(${PROJ_NAME} main.cpp )

# Import sources
include(FetchContent)
FetchContent_Declare(
    sqlitefs_external
    GIT_REPOSITORY https://gitlab.com/p34ch-main/sqlitefs.git
    GIT_TAG main   # put the latest version tag
)
FetchContent_MakeAvailable(sqlitefs_external)

# add to the project
target_link_libraries(${PROJECT_NAME} sqlitefs)
```

```cpp
// main.cpp
#include <sqlitefs/sqlitefs.h>

int main(){
    SQLiteFS fs;
}
```

### Configuration

Use options to configure project

```cmake
# to compress files while writing to the db
option(MZ_ENABLE "Enables minizip lib" ON)
option(MZ_ZLIB "Enables ZLIB compression" ON)
option(MZ_BZIP2 "Enables BZIP2 compression" ON)
option(MZ_LZMA "Enables LZMA & XZ compression" ON)
option(MZ_ZSTD "Enables ZSTD compression" ON)

# AES128 AES256 CHACHA20 SQLCIPHER RC4 ASCON128
set(CODEC_TYPE AES256 CACHE STRING "Set default codec type")
```

## SAST Tools

[PVS-Studio](https://pvs-studio.com/en/pvs-studio/?utm_source=website&utm_medium=github&utm_campaign=open_source) - static analyzer for C, C++, C#, and Java code.

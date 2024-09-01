# SQLiteFS

Simulate a local fs inside a sqlite database.

## Support data modifications

You can set a callback to modify the data before storing the data in the DB, such as compression or encryption, and another callback to decompress or decrypt. You can also combine them (zip+aes).

Check [zlib](https://gitlab.com/p34ch-main/sqlitefs/-/tree/zlib?ref_type=heads) and [minizip](https://gitlab.com/p34ch-main/sqlitefs/-/tree/minizip?ref_type=heads) branches to have compression out of box.

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

### Example

```cpp
    std::string               data("random test data");
    std::vector<std::uint8_t> content{data.begin(), data.end()};

    SQLiteFS fs("./fs.db");
    fs.mkdir("folder");
    fs.cd("folder");

    fs.write("test.txt", content, "raw");
    auto read_data = fs.read("test.txt");

    ASSERT_EQ(read_data, content);
```

Check tests for more examples

## How to include into your project

### Import precompiled libs (Windows only)

```cmake
include(FetchContent)

FetchContent_Declare(
    sqlitefs_external
    DOWNLOAD_EXTRACT_TIMESTAMP true

    URL       https://gitlab.com/api/v4/projects/61159815/packages/generic/sqlitefs/v1.0.1/sqlitefs-v1.0.1.zip
    # or with compression lib
    # URL       https://gitlab.com/api/v4/projects/61159815/packages/generic/sqlitefs/v1.0.1-minizip/sqlitefs-v1.0.1-minizip.zip
)
FetchContent_MakeAvailable(sqlitefs_external)
```

### Import source

```cmake
include(FetchContent)

FetchContent_Declare(
    sqlitefs_external
    GIT_REPOSITORY https://gitlab.com/p34ch-main/sqlitefs.git

    GIT_TAG v1.0.1
    # or with compression lib
    # GIT_TAG v1.0.1-zlib
    # GIT_TAG v1.0.1-minizip
)

FetchContent_MakeAvailable(sqlitefs_external)
```

### Main CMakeLists

```cmake
target_link_libraries(${PROJECT_NAME} sqlitefs)
```

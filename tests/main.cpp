#include <filesystem>
#include <gtest/gtest.h>
#include <sqlitefs/sqlitefs.h>


class SQLiteFSTestFixture : public testing::Test {
protected:
    void SetUp() override { db = std::make_unique<SQLiteFS>(db_path); }

    void TearDown() override {
        db.reset();
        std::filesystem::remove(db_path);
    }

    std::string               db_path = "./test_db.db";
    std::unique_ptr<SQLiteFS> db;
};


TEST_F(SQLiteFSTestFixture, GetRoot) {
    ASSERT_EQ(db->pwd(), "/");
}


TEST_F(SQLiteFSTestFixture, CreateFolder) {
    std::vector<SQLiteFSNode> expected;

    ASSERT_EQ(db->pwd(), "/");
    ASSERT_EQ(db->ls(), expected);

    std::string folder1 = "folder1";
    expected.emplace_back(SQLiteFSNode{.id = 1, .parent_id = 0, .name = folder1}); // NOLINT

    ASSERT_TRUE(db->mkdir(folder1));
    ASSERT_EQ(db->ls(), expected);

    ASSERT_FALSE(db->mkdir(folder1));
    ASSERT_EQ(db->ls(), expected);

    std::string folder2 = "folder2";
    expected.emplace_back(SQLiteFSNode{.id = 2, .parent_id = 0, .name = folder2}); // NOLINT

    ASSERT_TRUE(db->mkdir(folder2));
    ASSERT_EQ(db->ls(), expected);

    std::string subfolder = "/" + folder2 + "/" + folder1;

    ASSERT_TRUE(db->mkdir(subfolder));
    ASSERT_TRUE(db->cd(folder2));

    std::vector<SQLiteFSNode> expected2{{.id = 3, .parent_id = 2, .name = folder1}}; // NOLINT
    ASSERT_EQ(db->ls(), expected2);
    ASSERT_EQ(db->ls("."), expected2);
    ASSERT_EQ(db->ls(".."), expected);
}


TEST_F(SQLiteFSTestFixture, CreateRemoveFolder) {
    std::vector<SQLiteFSNode> expected;

    std::string folder1 = "folder1";
    expected.emplace_back(SQLiteFSNode{.id = 1, .parent_id = 0, .name = folder1}); // NOLINT

    ASSERT_TRUE(db->mkdir(folder1));
    ASSERT_EQ(db->ls(), expected);

    ASSERT_TRUE(db->cd(folder1));
    ASSERT_TRUE(db->mkdir(folder1));
    expected[0].id        = 2;
    expected[0].parent_id = 1;
    ASSERT_EQ(db->ls(), expected);

    expected.clear();

    ASSERT_TRUE(db->rm("/" + folder1 + "/" + folder1));
    ASSERT_EQ(db->ls(), expected);

    ASSERT_FALSE(db->rm(folder1));
    ASSERT_EQ(db->ls(), expected);
}

TEST_F(SQLiteFSTestFixture, ChangeFolder) {
    std::vector<std::string> expected;

    std::string folder1 = "folder1";
    expected.push_back(folder1);

    ASSERT_EQ(db->pwd(), "/");
    ASSERT_TRUE(db->mkdir("f1"));
    ASSERT_TRUE(db->mkdir("f2"));

    ASSERT_TRUE(db->cd("f2"));

    ASSERT_EQ(db->pwd(), "/f2");

    ASSERT_FALSE(db->cd("f1"));
    ASSERT_TRUE(db->mkdir("f1"));
    ASSERT_TRUE(db->cd("f1"));
    ASSERT_EQ(db->pwd(), "/f2/f1");

    ASSERT_TRUE(db->cd(".."));
    ASSERT_EQ(db->pwd(), "/f2");

    ASSERT_TRUE(db->cd(".."));
    ASSERT_EQ(db->pwd(), "/");

    ASSERT_TRUE(db->cd("f2/f1"));
    ASSERT_EQ(db->pwd(), "/f2/f1");

    ASSERT_TRUE(db->cd("/f1"));
    ASSERT_EQ(db->pwd(), "/f1");

    ASSERT_TRUE(db->cd("/"));
    ASSERT_EQ(db->pwd(), "/");
}


TEST_F(SQLiteFSTestFixture, PutFile) {
    ASSERT_EQ(db->pwd(), "/");

    std::string               data("random test data");
    std::vector<std::uint8_t> content{data.begin(), data.end()};

    ASSERT_TRUE(db->put("test.txt", content));
    ASSERT_FALSE(db->put("test.txt", content));

    db->mkdir("f1");
    db->mkdir("f1/f2");

    ASSERT_TRUE(db->put("f1/f2/test.txt", content));
    ASSERT_FALSE(db->put("f1/f2/test.txt", content));

    db->cd("f1");
    ASSERT_TRUE(db->put("/f1/f2/test2.txt", content));
    ASSERT_TRUE(db->put("../test2.txt", content));
}


TEST_F(SQLiteFSTestFixture, PutGetFile) {
    ASSERT_EQ(db->pwd(), "/");

    std::string               data("random test data");
    std::vector<std::uint8_t> content{data.begin(), data.end()};
    {
        ASSERT_TRUE(db->put("test.txt", content, "zlib"));
        auto read_data = db->get("test.txt");

        ASSERT_EQ(read_data.size(), content.size());
        ASSERT_EQ(read_data, content);
    }

    {
        ASSERT_TRUE(db->put("test2.txt", content, "raw"));
        auto read_data = db->get("test2.txt");

        ASSERT_EQ(read_data.size(), content.size());
        ASSERT_EQ(read_data, content);
    }

    {
        db->mkdir("f1");
        db->mkdir("f1/f2");

        ASSERT_TRUE(db->put("f1/f2/test.txt", content));
        auto read_data = db->get("f1/f2/test.txt");

        ASSERT_EQ(read_data.size(), content.size());
        ASSERT_EQ(read_data, content);
    }
}

TEST_F(SQLiteFSTestFixture, PutGetFileWithCompression) {
    ASSERT_EQ(db->pwd(), "/");

    std::string               data("random test data");
    std::vector<std::uint8_t> content{data.begin(), data.end()};

    {
        ASSERT_TRUE(db->put("test.txt", content));
        auto read_data = db->get("test.txt");

        ASSERT_EQ(read_data.size(), content.size());
        ASSERT_EQ(read_data, content);
    }

    {
        ASSERT_TRUE(db->put("compressed.txt", content, "zlib"));
        auto read_data = db->get("compressed.txt");

        ASSERT_EQ(read_data.size(), content.size());
        ASSERT_EQ(read_data, content);
    }
}


TEST_F(SQLiteFSTestFixture, PutGetFileWithCustomConvertFunction) {
    ASSERT_EQ(db->pwd(), "/");

    db->registerSaveFunc("myFunc",
                         [](SQLiteFS::DataInput data) { return SQLiteFS::DataOutput{data.rbegin(), data.rend()}; });

    db->registerLoadFunc("myFunc",
                         [](SQLiteFS::DataInput data) { return SQLiteFS::DataOutput{data.rbegin(), data.rend()}; });

    std::string               data("random test data");
    std::vector<std::uint8_t> content{data.begin(), data.end()};

    {
        ASSERT_TRUE(db->put("compressed.txt", content, "myFunc"));
        auto read_data = db->get("compressed.txt");

        ASSERT_EQ(read_data.size(), content.size());
        ASSERT_EQ(read_data, content);
    }
}


TEST_F(SQLiteFSTestFixture, PutGetFileWithComplexCustomConvertFunction) {
    ASSERT_EQ(db->pwd(), "/");

    db->registerSaveFunc("myComplexFunc", [&](SQLiteFS::DataInput data) {
        auto packed = db->callSaveFunc("zlib", data);
        return SQLiteFS::DataOutput{packed.rbegin(), packed.rend()};
    });
    db->registerLoadFunc("myComplexFunc", [&](SQLiteFS::DataInput data) {
        auto still_packed = SQLiteFS::DataOutput{data.rbegin(), data.rend()};
        return db->callLoadFunc("zlib", still_packed);
    });

    std::string               data("random test data");
    std::vector<std::uint8_t> content{data.begin(), data.end()};

    {
        ASSERT_TRUE(db->put("compressed.txt", content, "myComplexFunc"));
        auto read_data = db->get("compressed.txt");

        ASSERT_EQ(read_data.size(), content.size());
        ASSERT_EQ(read_data, content);
    }
}
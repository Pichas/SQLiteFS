#include <filesystem>
#include <gtest/gtest.h>
#include <sqlitefs/sqlitefs.h>


class FSFixture : public testing::Test {
protected:
    void SetUp() override { db = std::make_unique<SQLiteFS>(db_path); }

    void TearDown() override {
        db.reset();
        std::filesystem::remove(db_path);
    }

    std::string               db_path = "./test_db.db";
    std::unique_ptr<SQLiteFS> db;
};


TEST_F(FSFixture, GetRoot) {
    ASSERT_EQ(db->pwd(), "/");
}


TEST_F(FSFixture, GetDeletedFolder) {
    ASSERT_EQ(db->pwd(), "/");
    ASSERT_TRUE(db->mkdir("f1"));
    ASSERT_TRUE(db->cd("f1"));
    ASSERT_EQ(db->pwd(), "/f1");
    ASSERT_TRUE(db->rm("/f1"));
    ASSERT_EQ(db->pwd(), "/");
}


TEST_F(FSFixture, CreateFolder) {
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

    std::string               data("random test data");
    std::vector<std::uint8_t> content{data.begin(), data.end()};
    ASSERT_TRUE(db->put("test.txt", content));

    expected.clear();
    ASSERT_EQ(db->ls("test.txt"), expected);
}


TEST_F(FSFixture, CreateRemoveFolder) {
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

    ASSERT_TRUE(db->rm("/" + folder1));
}


TEST_F(FSFixture, ChangeFolder) {
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


TEST_F(FSFixture, PutFile) {
    ASSERT_EQ(db->pwd(), "/");

    std::string               data("random test data");
    std::vector<std::uint8_t> content{data.begin(), data.end()};

    ASSERT_TRUE(db->put("test.txt", content));
    ASSERT_FALSE(db->put("test.txt", content));
    ASSERT_FALSE(db->put("/f1/test.txt", content));
    ASSERT_FALSE(db->put("../../../test.txt", content));

    auto files = db->ls();
    ASSERT_EQ(files.size(), 1);
    ASSERT_EQ(files[0].id, 1);
    ASSERT_EQ(files[0].parent_id, 0);
    ASSERT_EQ(files[0].name, "test.txt");
    ASSERT_GT(files[0].size, 0);
    ASSERT_GT(files[0].size_raw, 0);
    ASSERT_TRUE(files[0].is_file);


    db->mkdir("f1");
    db->mkdir("f1/f2");

    ASSERT_TRUE(db->put("f1/f2/test.txt", content));
    ASSERT_FALSE(db->put("f1/f2/test.txt", content));

    db->cd("f1");
    ASSERT_TRUE(db->put("/f1/f2/test2.txt", content));
    ASSERT_TRUE(db->put("../test2.txt", content));
}


TEST_F(FSFixture, PutGetFile) {
    ASSERT_EQ(db->pwd(), "/");

    std::string               data("random test data");
    std::vector<std::uint8_t> content{data.begin(), data.end()};
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


TEST_F(FSFixture, PutGetFileWithModification) {
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
        db->registerSaveFunc("reverse",
                             [](SQLiteFS::DataInput data) { return SQLiteFS::DataOutput{data.rbegin(), data.rend()}; });

        db->registerLoadFunc("reverse",
                             [](SQLiteFS::DataInput data) { return SQLiteFS::DataOutput{data.rbegin(), data.rend()}; });

        ASSERT_TRUE(db->put("test2.txt", content, "reverse"));
        auto read_data = db->get("test2.txt");

        ASSERT_EQ(read_data.size(), content.size());
        ASSERT_EQ(read_data, content);
    }
}


TEST_F(FSFixture, PutGetFileWithCustomConvertFunction) {
    ASSERT_EQ(db->pwd(), "/");

    db->registerSaveFunc("reverse",
                         [](SQLiteFS::DataInput data) { return SQLiteFS::DataOutput{data.rbegin(), data.rend()}; });

    db->registerLoadFunc("reverse",
                         [](SQLiteFS::DataInput data) { return SQLiteFS::DataOutput{data.rbegin(), data.rend()}; });

    std::string               data("random test data");
    std::vector<std::uint8_t> content{data.begin(), data.end()};

    {
        ASSERT_TRUE(db->put("test.txt", content, "reverse"));
        auto read_data = db->get("test.txt");

        ASSERT_EQ(read_data.size(), content.size());
        ASSERT_EQ(read_data, content);
    }
}


TEST_F(FSFixture, PutGetFileWithComplexComplexConvertFunction) {
    ASSERT_EQ(db->pwd(), "/");

    db->registerSaveFunc("myComplexFunc", [&](SQLiteFS::DataInput data) {
        auto modified_data = db->callSaveFunc("raw", data);
        return SQLiteFS::DataOutput{modified_data.rbegin(), modified_data.rend()};
    });
    db->registerLoadFunc("myComplexFunc", [&](SQLiteFS::DataInput data) {
        auto still_packed = SQLiteFS::DataOutput{data.rbegin(), data.rend()};
        return db->callLoadFunc("raw", still_packed);
    });

    std::string               data("random test data");
    std::vector<std::uint8_t> content{data.begin(), data.end()};

    {
        ASSERT_TRUE(db->put("test.txt", content, "myComplexFunc"));
        auto read_data = db->get("test.txt");

        ASSERT_EQ(read_data.size(), content.size());
        ASSERT_EQ(read_data, content);
    }
}


TEST_F(FSFixture, MoveFileOrFolder) {
    ASSERT_EQ(db->pwd(), "/");
    ASSERT_TRUE(db->mkdir("f1"));
    ASSERT_TRUE(db->mkdir("f2"));


    std::string               data("random test data");
    std::vector<std::uint8_t> content{data.begin(), data.end()};
    ASSERT_TRUE(db->put("/f1/test.txt", content));

    {
        auto read_data = db->get("/f1/test.txt");
        ASSERT_EQ(read_data.size(), content.size());
        ASSERT_EQ(read_data, content);
    }

    {
        auto read_data = db->get("/f2/test2.txt");
        ASSERT_EQ(read_data.size(), 0);
    }

    ASSERT_FALSE(db->mv("/f1/test.txt", "/f3/test2.txt"));
    ASSERT_TRUE(db->mv("/f1/test.txt", "/f2/test2.txt"));
    ASSERT_FALSE(db->mv("/f1/test.txt", "/f2/test2.txt"));

    {
        auto read_data = db->get("/f1/test.txt");
        ASSERT_EQ(read_data.size(), 0);
    }

    {
        auto read_data = db->get("/f2/test2.txt");
        ASSERT_EQ(read_data.size(), content.size());
        ASSERT_EQ(read_data, content);
    }

    ASSERT_FALSE(db->mv("/f2", "/f1"));
    ASSERT_TRUE(db->mv("/f2", "/f1/f3"));

    {
        auto read_data = db->get("/f1/f3/test2.txt");
        ASSERT_EQ(read_data.size(), content.size());
        ASSERT_EQ(read_data, content);
    }
}


TEST_F(FSFixture, CopyFile) {
    ASSERT_EQ(db->pwd(), "/");
    ASSERT_TRUE(db->mkdir("f1"));
    ASSERT_TRUE(db->mkdir("f2"));


    std::string               data("random test data");
    std::vector<std::uint8_t> content{data.begin(), data.end()};
    ASSERT_TRUE(db->put("/f1/test.txt", content));

    {
        auto read_data = db->get("/f1/test.txt");
        ASSERT_EQ(read_data.size(), content.size());
        ASSERT_EQ(read_data, content);
    }

    {
        auto read_data = db->get("/f2/test2.txt");
        ASSERT_EQ(read_data.size(), 0);
    }

    ASSERT_FALSE(db->cp("/f1", "/f2"));
    ASSERT_FALSE(db->cp("/f1", "/f2/test2.txt"));
    ASSERT_FALSE(db->cp("/f1/test.txt", "/f3/test2.txt"));
    ASSERT_TRUE(db->cp("/f1/test.txt", "/f2/test2.txt"));
    ASSERT_FALSE(db->cp("/f1/test.txt", "/f2/test2.txt"));

    {
        auto read_data = db->get("/f1/test.txt");
        ASSERT_EQ(read_data.size(), content.size());
        ASSERT_EQ(read_data, content);
    }


    {
        auto read_data = db->get("/f2/test2.txt");
        ASSERT_EQ(read_data.size(), content.size());
        ASSERT_EQ(read_data, content);
    }
}


TEST(Manual, manual) {
    std::string db_path = "./manual_test.db";
    std::filesystem::remove(db_path);
    auto db = std::make_unique<SQLiteFS>(db_path);

    ASSERT_EQ(db->pwd(), "/");
    ASSERT_TRUE(db->mkdir("f1"));
    ASSERT_TRUE(db->mkdir("f2"));

    std::string               data("random test data");
    std::vector<std::uint8_t> content{data.begin(), data.end()};
    ASSERT_TRUE(db->put("/f1/test.txt", content));
    ASSERT_TRUE(db->cp("/f1/test.txt", "/f2/test2.txt"));
}

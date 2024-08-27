#include <fstream>
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
    std::vector<std::string> expected;

    ASSERT_EQ(db->pwd(), "/");
    ASSERT_EQ(db->ls(), expected);

    std::string folder1 = "folder1";
    expected.push_back(folder1);

    ASSERT_TRUE(db->mkdir(folder1));
    ASSERT_EQ(db->ls(), expected);

    ASSERT_FALSE(db->mkdir(folder1));
    ASSERT_EQ(db->ls(), expected);

    std::string folder2 = "folder2";
    expected.push_back(folder2);

    ASSERT_TRUE(db->mkdir(folder2));
    ASSERT_EQ(db->ls(), expected);
}


TEST_F(SQLiteFSTestFixture, CreateRemoveFolder) {
    std::vector<std::string> expected;

    std::string folder1 = "folder1";
    expected.push_back(folder1);

    ASSERT_TRUE(db->mkdir(folder1));
    ASSERT_EQ(db->ls(), expected);

    expected.clear();

    ASSERT_TRUE(db->rm(folder1));
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
}


TEST_F(SQLiteFSTestFixture, PutGetFile) {
    ASSERT_EQ(db->pwd(), "/");

    std::ifstream fs("tests/test.txt", std::ios::in | std::ios::binary);
    ASSERT_TRUE(fs);

    std::vector<Byte> content{std::istreambuf_iterator<char>(fs), std::istreambuf_iterator<char>()};

    ASSERT_TRUE(db->put("test.txt", content));
    auto read_data = db->get("test.txt");

    ASSERT_EQ(read_data.size(), content.size());
    ASSERT_EQ(read_data, content);
}

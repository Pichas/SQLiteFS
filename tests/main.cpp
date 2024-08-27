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

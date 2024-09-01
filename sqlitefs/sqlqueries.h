#pragma once

#include <string>
#include <vector>


const inline std::vector<std::string> INIT_DB{
  R"query(
        CREATE TABLE IF NOT EXISTS "fs" (
            "id"          INTEGER,
            "parent"      INTEGER,
            "name"        TEXT NOT NULL,
            "attrib"      INTEGER DEFAULT 0,
            "size"        INTEGER,
            "size_raw"    INTEGER,
            "compression" TEXT,
            PRIMARY KEY("id" AUTOINCREMENT),
            UNIQUE("parent","name"),
            CONSTRAINT "parent_fk" FOREIGN KEY("parent") REFERENCES "fs"("id") ON UPDATE CASCADE ON DELETE CASCADE
        )
    )query",

  R"query(
        CREATE TABLE IF NOT EXISTS "data" (
            "id"    INTEGER,
            "data"  BLOB NOT NULL,
            PRIMARY KEY("id"),
            CONSTRAINT "file_id" FOREIGN KEY("id") REFERENCES "fs"("id") ON UPDATE CASCADE ON DELETE CASCADE
        )
    )query",

  R"query(INSERT OR IGNORE INTO fs ("id", "name") VALUES ('0','/'))query",
};

const inline std::string PWD = R"query(
        SELECT concat('/', group_concat(n, '/')) FROM (
            WITH RECURSIVE
            pwd(i, p, n) AS (
                SELECT id, parent, name FROM fs WHERE id IS ? and parent not NULL
                UNION ALL
                SELECT id, parent, name FROM fs, pwd WHERE fs.id IS pwd.p and parent not NULL
            )
            SELECT * FROM pwd ORDER BY i ASC
        )
    )query";

// clang-format off

const inline std::string LS            = R"query(SELECT * FROM fs WHERE parent IS ?)query";
const inline std::string GET_ID        = R"query(SELECT id FROM fs WHERE parent IS ? AND name IS ?)query";
const inline std::string GET_PARENT_ID = R"query(SELECT parent FROM fs WHERE id IS ?)query";
const inline std::string GET_INFO      = R"query(SELECT * FROM fs WHERE id IS ?)query";
const inline std::string RM            = R"query(DELETE FROM fs WHERE parent IS ? AND name IS ?)query";
const inline std::string MKDIR         = R"query(INSERT INTO fs (parent, name) VALUES (?, ?))query";

const inline std::string SET_PARENT_ID = R"query(UPDATE fs SET parent = ? WHERE id IS ?)query";
const inline std::string SET_NAME      = R"query(UPDATE fs SET name = ? WHERE id IS ?)query";

const inline std::string COPY_FILE_FS  = R"query(INSERT INTO fs (parent, name, attrib, size, size_raw, compression) SELECT ?, ?, attrib, size, size_raw, compression FROM fs WHERE id is ?;)query";
const inline std::string COPY_FILE_RAW = R"query(INSERT INTO data (id, data) SELECT ?, data FROM data WHERE id is ?;)query";




const inline std::string TOUCH         = R"query(INSERT INTO fs (parent, name, size, size_raw, compression, attrib) VALUES (?, ?, ?, ?, ?, 1))query";
const inline std::string SET_FILE_DATA = R"query(INSERT INTO data (id, data) VALUES (?, ?))query";
const inline std::string GET_FILE_DATA = R"query(SELECT data FROM data WHERE id IS ?)query";

// clang-format on
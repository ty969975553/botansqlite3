// smartsqlite_test.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <memory>
#include <iostream>
#include <array>
#define SQLITE_HAS_CODEC 1
#include "../src/smartsqlite/exceptions.h"
#include "../src/smartsqlite/version.h"
#include "../src/smartsqlite/blob.h"
#include "../src/smartsqlite/connection.h"
#include "../src/smartsqlite/scopedsavepoint.h"
//#pragma comment(lib,"botan.lib")

std::string SP_NAME = "test_savepoint";

namespace TestUtil {

    using ConnPtr = std::shared_ptr<SmartSqlite::Connection>;

    inline ConnPtr makeConnection() {
        return std::make_shared<SmartSqlite::Connection>(":memory:");
    }

    inline int getUserVersion(ConnPtr conn) {
        auto stmt = conn->prepare("PRAGMA user_version");
        return stmt.execWithSingleResult().get<int>(0);
    }

    inline void setUserVersion(ConnPtr conn, int version) {
        conn->exec("PRAGMA user_version = " + std::to_string(version));
    }

    //SmartSqlite::Blob open(
    //    std::int64_t rowid,
    //    SmartSqlite::Blob::Flags flags = SmartSqlite::Blob::READONLY) {
    //    return conn_.openBlob("main", "blobs", "data", rowid, flags);
    //}
}
using namespace TestUtil;

void test1() {
    //create db;  memory
    std::int64_t rowidZero_, rowidNonzero_;
    ConnPtr con = makeConnection();
    con->exec("CREATE TABLE blobs (data BLOB)");
    con->exec("INSERT INTO blobs VALUES (zeroblob(42))");
    rowidZero_ = con->lastInsertRowId();

    SmartSqlite::Blob b = con->openBlob("main", "blobs", "data", rowidZero_, SmartSqlite::Blob::READONLY);
    std::cout << b.size() << std::endl;

    con->exec("INSERT INTO blobs VALUES (x'0123456789abcdef')");
    rowidNonzero_ = con->lastInsertRowId();
    b = con->openBlob("main", "blobs", "data", rowidNonzero_, SmartSqlite::Blob::READONLY);
    std::cout << b.size() << std::endl;
    std::array<std::uint8_t, 8> buf;
    b.read(buf.data(), buf.size());
}

void test2() {
    //create test.db
    auto conn = std::make_shared<SmartSqlite::Connection>("test.db");

    conn->exec("PRAGMA journal_mode=WAL");
    conn->exec("DROP TABLE IF EXISTS foo");
    conn->exec("CREATE TABLE foo (bar INTEGER)");

    setUserVersion(conn, 23);
    SmartSqlite::ScopedSavepoint sp{ conn, SP_NAME };
    conn->exec("INSERT INTO foo VALUES (42)");
    conn->exec("SELECT * FROM foo");
    setUserVersion(conn, 42);

    sp.rollback();
    int version = getUserVersion(conn);

    conn->exec("INSERT INTO foo VALUES (42)");
    conn->exec("SELECT * FROM foo");
    //conn->rollbackToSavepoint(SP_NAME);
    version = getUserVersion(conn);
}

void test3() {
    std::string version = SmartSqlite::sqliteVersion();
    std::cout << version ;
}


void test4() {
    //create test.db
    auto conn = std::make_shared<SmartSqlite::Connection>("test2.db");
    conn->exec("DROP TABLE IF EXISTS all_types");
    conn->exec("CREATE TABLE all_types "
        "(c_int INT, c_float FLOAT, c_text TEXT, c_blob BLOB)");
    conn->exec("INSERT INTO all_types "
        "VALUES (42, 2.0, '6*7', x'deadbeef')");
    conn->exec("INSERT INTO all_types "
        "VALUES (NULL, NULL, NULL, NULL)");

    SmartSqlite::Statement statement = conn->prepare("SELECT * FROM all_types WHERE c_int IS NOT NULL");

    auto iter = statement.begin();
    std::size_t v = iter->get<std::size_t>(0);
    float f = iter->get<float>(1);
    std::string ss = iter->get<std::string>(2);
    std::vector<unsigned char> vec = iter->get<std::vector<unsigned char>>(3);
    for (auto item : vec)
    {

    }


    SmartSqlite::Statement statement2 = conn->prepare("SELECT c_text FROM all_types WHERE c_int = ?");
    std::string what;

    //try {
    //    statement2.execWithoutResult();
    //}
    //catch (SmartSqlite::QueryReturnedRows &ex) {
    //    what = ex.what();
    //}


    statement2.bindNull(0);
    bool value = true;
    statement2.bind(0, value);
    std::uint8_t value2 = 42;
    statement2.bind(0, value2);
    std::uint16_t value3 = 42;
    statement2.bind(0, value3);

    SmartSqlite::Statement stmt = conn->prepare(
        "INSERT INTO all_types "
        "VALUES (?, ?, ?, ?)");
    int pos = 0;
    stmt.bindNullable(pos++, SmartSqlite::Nullable<int>());
    stmt.bindNullable(pos++, SmartSqlite::Nullable<double>());
    stmt.bindNullable(pos++, SmartSqlite::Nullable<std::string>());
    stmt.bindNullable(pos++, SmartSqlite::Nullable<std::vector<unsigned char>>());

    SmartSqlite::Statement stmt2 = conn->prepare(
        "SELECT * FROM all_types "
        "WHERE c_int = :answer");
    stmt2.bind(":answer", 42);
    float queryvalue = stmt2.begin()->get<float>(1);
}

void test5() {
    auto conn = std::make_shared<SmartSqlite::Connection>("test3.db");
    std::string SOME_KEY =
        "MTIzNDU2Nzg5MDEyMzQ1Njc4OTAxMjM0"
        "NTY3ODkwMTIxMjM0NTY3ODkwMTIzNDU2"
        "Nzg5MDEyMzQ1Njc4OTAxMjEyMzQ1Njc4"
        "OTAxMjM0NTY3ODkwMTIzNDU2Nzg5MDEy";
    std::string KEY2 = "P4721k5DTj6gemvElcgVNX8BKZrp4CALW9TtmCEp130L298XZfecPqdR4W3IPh9bc16DSyz6zlSwT3f1BG9bYsH5fkFT7FuDoE4F02pzaXdjf88c3P3f2oE2D1Ca1DEW";
    //std::cout << SOME_KEY.size();
    //const char* key = "sfx0VPEyA6gL9vyMWT8tfEZMWyIqOqbKyOUbtRLknMjSG7aKiQGtHUbqXEsFxGnrmJwg3t0j";
    conn->setKey(SOME_KEY);
    conn->exec("DROP TABLE IF EXISTS all_types");
    conn->exec("CREATE TABLE all_types "
        "(c_int INT, c_float FLOAT, c_text TEXT, c_blob BLOB)");
    conn->exec("INSERT INTO all_types "
        "VALUES (42, 2.0, '6*7', x'deadbeef')");
    conn->exec("INSERT INTO all_types "
        "VALUES (NULL, NULL, NULL, NULL)");

    conn.reset();
    conn = std::make_shared<SmartSqlite::Connection>("test3.db");
    conn->setKey(SOME_KEY);

    SmartSqlite::Statement stmt2 = conn->prepare(
        "SELECT * FROM all_types "
        "WHERE c_int = :answer");
    stmt2.bind(":answer", 42);
    float queryvalue = stmt2.begin()->get<float>(1);
    std::cout << queryvalue << std::endl;
}

void test6() {
    std::string dbpath =    "E:\\ws\\smartsqlite2\\cache"; 
    auto conn = std::make_shared<SmartSqlite::Connection>(dbpath);
    std::string SOME_KEY =
        "MTIzNDU2Nzg5MDEyMzQ1Njc4OTAxMjM0"
        "NTY3ODkwMTIxMjM0NTY3ODkwMTIzNDU2"
        "Nzg5MDEyMzQ1Njc4OTAxMjEyMzQ1Njc4"
        "OTAxMjM0NTY3ODkwMTIzNDU2Nzg5MDEy";
    conn->setKey(SOME_KEY);
    SmartSqlite::Statement stmt2 = conn->prepare(
        "SELECT * FROM all_types "
        "WHERE c_int = :answer");
    stmt2.bind(":answer", 42);
    float queryvalue = stmt2.begin()->get<float>(1);
    std::cout << queryvalue << std::endl;
}

int main()
{
    //test1();
    //test2();
    //test3();
    //test4();
    //test5();
    test6();
    return 0;
}


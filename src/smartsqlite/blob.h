/*
* Copyright 2014-2017 The SmartSqlite contributors (see NOTICE.txt)
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*        http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#pragma once

#include <cstdint>
#include <memory>

struct sqlite3;
struct sqlite3_blob;

namespace SmartSqlite {

class Blob
{
public:
    enum Flags { READONLY = 0, READWRITE = 1 };

    explicit Blob(sqlite3 *conn, sqlite3_blob *blob);
    Blob(Blob &&other);
    Blob &operator=(Blob &&rhs);
    ~Blob();

    void moveToRow(std::int64_t rowid);
    size_t size() const;
    size_t read(void *buffer, size_t size, size_t offset = 0) const;
    size_t write(const void *buffer, size_t size, size_t offset = 0);

private:
    size_t getAccessSize(size_t bufferSize, size_t offset) const;

    struct Impl;
    std::unique_ptr<Impl> impl;
};

}

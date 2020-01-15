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
#include "smartsqlite/exceptions.h"

#include "result_names.h"
#include "smartsqlite/sqlite3.h"

namespace SmartSqlite {

Exception::Exception(const std::string &message, const std::string &sql) noexcept
    : m_message(message + (!sql.empty() ? " SQL: '" + sql + "'" : ""))
{
}

const char *Exception::what() const noexcept
{
    return m_message.c_str();
}

ParameterUnknown::ParameterUnknown(const std::string &parameter)
    : Exception(
          std::string("Parameter not found in query: ") +
          parameter)
{
}

ColumnUnknown::ColumnUnknown(int &columnPos)
    : Exception(
          std::string("Column not found in result: ") +
          std::to_string(columnPos))
{
}

ColumnUnknown::ColumnUnknown(const std::string &columnName)
    : Exception(
          std::string("Column not found in result: ") +
          columnName)
{
}

QueryReturnedRows::QueryReturnedRows(const std::string &sql)
    : Exception("The query shouldn't have returned rows, but it did.", sql)
{
}

QueryReturnedNoRows::QueryReturnedNoRows(const std::string &sql)
    : Exception("The query should have returned a row, but it didn't.", sql)
{
}

SqliteException::SqliteException(const std::string &func, int resultCode)
    : Exception(
          std::string("[") + func + "] " +
          resultToResultName(resultCode) +
          " (" + sqlite3_errstr(resultCode) + ")"
          )
{
}

SqliteException::SqliteException(const std::string &func, int resultCode, const std::string &message)
    : Exception(
          std::string("[") + func + "] " +
          resultToResultName(resultCode) +
          " (" + sqlite3_errstr(resultCode) + "): " +
          message
          )
{
}

FeatureUnavailable::FeatureUnavailable(const std::string &feature)
    : Exception(std::string("Feature unavailable: ") + feature)
{
}

}

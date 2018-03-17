#ifndef __MYSQL_CXX_H__
#define __MYSQL_CXX_H__
#include <boost/shared_ptr.hpp>
#include <memory>
#include <stdint.h>
#include <string>
#include <vector>
#include <stdlib.h>
#include <string.h>
#include <noncopyable.h>

class SqlCommand;
class ResultSet;
struct SqlResultVarType;

class DBConnection : NonCopyable {
 public:
  virtual ~DBConnection() {}
  /**
   * connect to database
   * @param host ip address in string of database
   * @param port port database server listened
   * @param user login name
   * @param passwd password
   * @param db_name default database name
   */
  virtual int Connect(const char *host, unsigned short port, const char *user,
                      const char *passwd, const char *db_name) = 0;

  /**
   * Execute the sql, and return lines affected(or error number)
   * @param sql sql that will be executed
   * @return lines affected. if lines < 0, that means the error number!!!
   */
  virtual int ExecSql(const char *sql, int32_t len = 0) = 0;
  /**
   * similar with ExecSql(const char*);
   */
  virtual int ExecSql(const SqlCommand &command) = 0;

  /**
   * return the new id that insert sql returned
   * if previous sql is not insert or return of ExecSql < 0, the behavior is not
   * defined.
   */
  virtual unsigned long long GetNewID() = 0;

  /**
   * Execute one select sql that should return a ResultSet.
   * @param sql sql will be executed
   * @return the result set of select
   */
  virtual boost::shared_ptr<ResultSet> ExecSelect(const char *sql, int32_t len = 0) = 0;

  /**
   * EscapeString, prevent sql injection
   * @param from orign string
   * @param length from string length
   */
  virtual std::string EscapeString(const char *from, unsigned long length) = 0;
  std::string EscapeString(const std::string &str) {
    return this->EscapeString(str.c_str(), str.length());
  }
};

/**
 * The Result Set of executing of a select sql
 */
class ResultSet {
 public:
  ResultSet() : error(0) {}

  virtual ~ResultSet() {}

  /**
   * get the Field Count of the result set
   * @return field count
   */
  virtual int FieldCount() const = 0;

  /**
   * get the row count of the result set
   * return row count
   */
  virtual int RowCount() const = 0;

  /**
   * move the Cursor to the next row
   */
  virtual void Next() const = 0;

  /**
   * check current row is valid
   * @return true, valid; fale, invalid, should break.
   */
  virtual bool IsValid() const = 0;

  /**
   * get one field in current row, SqlResultVarType can convert to other
   * type implicitly, or you can get the raw data. @see SqlResultVarType
   * @param field field index, offset from zero
   * @return the field value @see SqlResultVarType
   */
  virtual SqlResultVarType operator[](int field) const = 0;
  virtual SqlResultVarType at(int field) const = 0;

  //error no
  int32_t error;
};

/**
 * VarType that can implicitly convert to other type, such as
 * char/short/int/long/long long/char*, etc.
 * if current value is blob, you can use GetRawData.
 * YOU CANNOT HOLD any pointer of this class. This class DO NOT manage the
 * resources.
 */
struct SqlResultVarType {
  SqlResultVarType(const char *data, int len) : data(data ? data : ""), len(len) {}

  bool is_valid() const { return data && len; }

  int32_t to_int32() const { return atoi(data); }

  uint32_t to_uint32() const { return atoi(data); }

  int64_t to_int64() const { return atoll(data); }

  uint64_t to_uint64() const { return atoll(data); }

  float to_float() const { return atof(data); }

  double to_double() const { return atof(data); }

  char* to_c_str() const { return const_cast<char *>(data); }

  std::string to_str() const { return std::string(data, data + len); }

  int copy(void *out, int l) {
    memcpy(out, data, len < l ? len : l);
    return len < l ? len : l;
  }

  const char *data;
  const int len;
};

/**
 * sql param type, prevent sql injection
 */
struct SqlParamType {
  int param_type;
  std::string data;
  void *ptr() const {
    return static_cast<void *>(const_cast<char *>(data.c_str()));
  }
  size_t size() const { return data.size(); }
};

/**
 * the command of a complex sql, which maybe have dangerous strings.
 * you can use it like this:
 *
 * MySqlCommand command("insert `tb`(`name`, `other`) values (?,?); ");
 * command.SetParam(0, "egmkang", strlen("egmkang"));
 * command.SetParam(1, "other", strlen("other"));
 */
class SqlCommand : NonCopyable {
 public:
  /**
   * @param sql which sql will be executed.
   */
  SqlCommand(const char *sql) : sql_(sql) {}
  virtual ~SqlCommand() {}

  /**
   * set the param the sql needed.
   * Only Support VARCHAR and BLOB
   * @param index which param will be replaced
   * @param len param's length
   */
  virtual SqlCommand &SetParam(int index, void *data, int len) = 0;
  virtual SqlCommand &SetParam(int index, const char *data, int len) = 0;

  const std::string &GetSql() const { return sql_; }
  const std::vector<SqlParamType> &GetParams() const { return params; }

 protected:
  std::string sql_;
  std::vector<SqlParamType> params;
};

#include <mysql/mysql.h>

/**
 * MySql's Command impl, @see SqlCommand
 */
class MySqlCommand : public SqlCommand {
 public:
  MySqlCommand(const char *sql) : SqlCommand(sql) {}
  ~MySqlCommand() {}

  virtual SqlCommand &SetParam(int index, void *data, int len);
  virtual SqlCommand &SetParam(int index, const char *data, int len);

 private:
  void SetParam(int index, int type, const void *data, int len);
};

/**
 * MySql's DBConnection impl, @see DBConnection
 */
class MySqlConnection : public DBConnection {
 public:
  MySqlConnection();
  ~MySqlConnection();

  bool IsReady() const { return is_ready_; }
  virtual int Connect(const char *host, unsigned short port, const char *user,
                      const char *passwd, const char *db_name);
  virtual int ExecSql(const char *sql, int32_t len = 0);
  virtual int ExecSql(const SqlCommand &command);
  virtual unsigned long long GetNewID();
  int32_t SkipNextResult();

  virtual boost::shared_ptr<ResultSet> ExecSelect(const char *sql, int32_t len = 0);

  using DBConnection::EscapeString;
  virtual std::string EscapeString(const char *from, unsigned long length);
  std::string GetLastError();

 private:
  MYSQL conn_;
  bool is_ready_;
};

/**
 * MySql's ResultSet impl, @see ResultSet
 */
class MySqlResultSet : public ResultSet {
 public:
  MySqlResultSet(int32_t error);
  MySqlResultSet(MYSQL_RES *res);
  ~MySqlResultSet();

  virtual int FieldCount() const;
  virtual int RowCount() const;
  virtual void Next() const;
  virtual bool IsValid() const;

  virtual SqlResultVarType operator[](int field) const;
  virtual SqlResultVarType at(int field) const;

 private:
  boost::shared_ptr<MYSQL_RES> result;
  mutable MYSQL_ROW row;
  mutable unsigned long *field_len;
  int total_fields;
  int total_rows;
  mutable int current_row;
};

#endif

#!/usr/bin/python3
#coding=utf-8

from mysql_config import *
from datetime import timedelta, datetime


conn = NewLogDBConn()
def GetLogTableName():
    ret = list()
    cur = conn.cursor()
    cur.execute('show tables like "%@date@"')
    for row in cur:
        ret.append(row[0])
    cur.close()
    return ret

table_template = list()

for table in GetLogTableName():
    cur = conn.cursor()
    cur.execute('show create table `%s`' % table)
    for row in cur:
        table_template.append(row[1])
    cur.close()

day = datetime.now() + timedelta(days=1)
day_str = "%04d-%02d-%02d" % (day.year, day.month, day.day)

for table_sql in table_template:
    sql = table_sql.replace('@date@', day_str).replace("CREATE TABLE", "CREATE TABLE IF NOT EXISTS")
    cur = conn.cursor()
    cur.execute(sql)
    #print(sql)
    for row in cur:
        pass
    cur.close()

conn.close()

#!/usr/bin/python3
# coding=utf-8

import sys
from mysql_config import *
from datetime import timedelta, datetime

if len(sys.argv) > 1:
    try:
        time_point = datetime.strptime(sys.argv[1], "%Y-%m-%d")
    except ValueError:
        time_point = datetime.now() - timedelta(days=1)
else:
    time_point = datetime.now() - timedelta(days=1)

conn = NewLogDBConn()
cur = conn.cursor()
day = (time_point - timedelta(days=60)).strftime("%Y-%m-%d")

cur.execute('show tables like "%@date@"')
result = cur.fetchall()
for row in result:
    table_name = row[0].replace('@date@', day)
    sql = "drop table `%s`" % table_name
    try:
        cur.execute(sql)
    except pymysql.err.InternalError as e:
        #print(e)
        pass

cur.close()
conn.close()

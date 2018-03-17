#!/usr/bin/python3
#coding=utf-8

from mysql_config import *
from datetime import timedelta, datetime

conn = NewLogDBConn()

day = datetime.now() - timedelta(days=1)
day_str = "%04d-%02d-%02d" % (day.year, day.month, day.day)

money_type = [1, 2, 5, 6, 7, 8, 9, 10, 11, 12]

#(server, currency) => dict( (system, msgid) => (income, cost) )
currency_dict = dict()

sql_template = """
select server, system, msgid, sum(delta_%s) as delta
from `currency_%s`
where delta_%s %s 0 group by server, system, msgid;
"""

def update_currency(currency, row):
    (server, system, msgid, delta) = (int(row[0]), int(row[1]), int(row[2]), int(row[3]))
    if (server, currency) not in currency_dict:
        currency_dict[(server, currency)] = dict()
    if (system, msgid) not in currency_dict[(server, currency)]:
        currency_dict[(server, currency)][(system, msgid)] = (0, 0)
    (income, cost) = currency_dict[(server, currency)][(system, msgid)]
    if delta > 0:
        income += delta
    else:
        cost += delta
    currency_dict[(server, currency)][(system, msgid)] = (income, cost)


for type in money_type:
    for cost_type in ['<', '>']:
        sql = sql_template % (type, day_str, type, cost_type)
        cur = conn.cursor()
        try:
            cur.execute(sql)
            for row in cur:
                update_currency(type, row)
        except:
            pass
        finally:
            cur.close()

conn_gm = NewGMDBConn()
cur = conn_gm.cursor()

try:
    sql_list = list()
    sql_list.append("insert into stat_currency (date, server, money_type, system, msgid, earn, cost) values ")
    server_keys = sorted(currency_dict.keys())
    for (server, currency) in server_keys:
        system_keys = sorted(currency_dict[(server, currency)].keys())
        for (system, msgid) in system_keys:
            (income, cost) = currency_dict[(server, currency)][(system, msgid)]
            if len(sql_list) > 1:
                sql_list.append(",")
            sql_list.append("('%s',%s,%s,%s,%s,%s,%s)" % (day_str, server, currency, system, msgid, income, cost))
    sql_list.append('ON DUPLICATE KEY UPDATE earn=values(earn), cost=values(cost)')
    sql = "".join(sql_list)
    cur.execute(sql)
except:
    pass
finally:
    cur.close()

conn_gm.close()
conn.close()
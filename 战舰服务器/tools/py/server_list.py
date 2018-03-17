#!/usr/bin/python3
#coding=utf-8

from mysql_config import *
import xml.dom.minidom
import os

gm_db_conn = NewGMDBConn()
gm_cur = gm_db_conn.cursor()

platform_list = [""]

server_list = dict()
sql = "select server_id, server_name, server_ip, server_port, server_start_time, maintain_time_begin, " \
      "maintain_time_end, client_version, client_channel from `server`"
try:
    gm_cur.execute(sql)
    for row in gm_cur:
        platform = str(int(row[0]) // 10000)
        platform_list.append(platform)
        if platform not in server_list:
            server_list[platform] = list()
        temp = dict()
        temp["server_id"] = str(row[0])
        temp["server_name"] = str(row[1])
        temp["server_ip"] = str(row[2])
        temp["server_port"] = str(row[3])
        temp["server_start_time"] = str(row[4])
        temp["maintain_time_begin"] = str(row[5])
        temp["maintain_time_end"] = str(row[6])
        temp["client_version"] = str(row[7]) if row[7] else ''
        temp["client_channel"] = str(row[8]) if row[8] else ''
        server_list[platform].append(temp)
except pymysql.err.IntegrityError as e:
    print(e)


def create_xml(platform_id):
    doc = xml.dom.minidom.Document()
    root = doc.createElement("server_list")
    doc.appendChild(root)
    for key in sorted(server_list.keys()):
        if platform_id != "" and platform_id != key and key != "3":
            continue
        pf_node = doc.createElement("platform")
        pf_node.setAttribute("platform_id", key)
        for item in server_list[key]:
            server_info = doc.createElement("server_info")
            for k in item:
                server_info.setAttribute(k, item[k])
            pf_node.appendChild(server_info)
        root.appendChild(pf_node)
    fp = open("/usr/share/nginx/html/platform_t%s.xml" % platform_id, "w", encoding="utf8")
    doc.writexml(fp)
    fp.close()
    os.system("mv -f /usr/share/nginx/html/platform_t%s.xml /usr/share/nginx/html/platform%s.xml" %
              (platform_id, platform_id))

for val in platform_list:
    create_xml(val)

gm_cur.close()
gm_db_conn.close()

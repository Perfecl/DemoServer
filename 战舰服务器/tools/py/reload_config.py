#!/usr/local/bin/python3
import os
import glob

#更新所有服务器的配置文件
#ps x | grep RecordServer$ | awk '{print $1}' | xargs kill -USR1

#解压config
print("unzip config")
os.system("cp config.tar.bz2 ./server/config -f")
os.system("cd ./server/config && tar -xf config.tar.bz2")

logic_server_list = glob.glob("./server/[0-9]*")
if len(logic_server_list) >= 1:
    for logic in logic_server_list:
        os.system("cp ./server/config/config/*.* %s/config/ -f" % logic)
        print("%s config" % logic)
    pass

print("kill -USR1 LogicServer")
print(os.system("ps x | grep LogicServer$ | awk '{print $5}'"))
os.system("ps x | grep LogicServer$ | awk '{print $1}' | xargs kill -USR1")

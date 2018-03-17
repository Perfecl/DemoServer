#!/usr/bin/python3
import os
from datetime import timedelta, datetime

if os.fork() > 0: os._exit(0)

while True:
    exec_set = set()
    with open("./watch.list", "r") as file:
        for line in file:
            exec_set.add(line.replace("\n", ""))
        #print(exec_set)
    os.system("sleep 10")
    os.system("ps aux | grep Server$ > temp.txt")
    
    current_proc_list = []
    with open("temp.txt", "r") as file:
        for line in file:
            current_proc_list.append(line.replace("\n", ""))
    
    for exec in exec_set:
        found = False
        for proc in current_proc_list:
            if proc.find(exec) >= 0:
                found = True
        if not found:
            with open("./daemon.log", "a+", encoding='utf8') as file:
                file.write("%s, %s, %s\n" % (datetime.now(), exec, found))
            try:
                path = "/".join(exec.split("/")[:-1])
                os.system("cd %s" % path)
                os.system("%s" % exec)
            except:
                pass

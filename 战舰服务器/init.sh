#! /bin/sh
mkdir -p message/cpp
mkdir -p bin/
svn co svn://192.168.16.100/mbgame03/doc/SE/04.配置文件/server config
svn sw svn://192.168.16.100/mbgame03/code/common/message  message/message --ignore-ancestry
cd ./deps/google && ./install.sh

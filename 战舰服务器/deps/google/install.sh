#!/bin/sh

mkdir -p ~/bin
mkdir -p ~/lib

rm -f ~/bin/protoc
cp ./protoc ~/bin

rm -f ~/lib/libprotoc.so*
rm -f ~/lib/libprotobuf.so*

cp ./lib/libprotoc.so.9.0.1 ~/lib
ln -s ~/lib/libprotoc.so.9.0.1 ~/lib/libprotoc.so.9
ln -s ~/lib/libprotoc.so.9.0.1 ~/lib/libprotoc.so

cp ./lib/libprotobuf.so.9.0.1 ~/lib
ln -s ~/lib/libprotobuf.so.9.0.1 ~/lib/libprotobuf.so.9
ln -s ~/lib/libprotobuf.so.9.0.1 ~/lib/libprotobuf.so

echo "export PATH=$PATH:$HOME/bin" >> ~/.bashrc
echo "export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$HOME/lib" >> ~/.bashrc
source ~/.bashrc

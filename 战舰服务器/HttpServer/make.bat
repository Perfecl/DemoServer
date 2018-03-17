cd HttpServer\message
..\..\bin\protoc --plugin=protoc-gen-sharpnet=..\..\bin\protoc-gen-sharpnet.exe --sharpnet_out . --proto_path "." message.proto 
..\..\bin\protoc --plugin=protoc-gen-sharpnet=..\..\bin\protoc-gen-sharpnet.exe --sharpnet_out . --proto_path "." server_message.proto 

cd ..\..

cd HttpServer
python parse.py

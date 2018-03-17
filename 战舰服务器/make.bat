cd message
python parse.py 

cd ..

cd message/message
protoc --cpp_out=../cpp ./*.proto
cd ../../

cd vsproject
cmake .. -G "Visual Studio 15 2017"
pause
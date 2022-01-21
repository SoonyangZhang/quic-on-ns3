# quic-on-ns3
running google quic on ns3  
This repo depends on quiche library.   
The quiche libarary can be got here: https://github.com/bilibili/quiche  
I make some minor changes on CmakeList.txt to get shared library.  
I upload the source code of quiche library on BaiduYunPan.  
url: https://pan.baidu.com/s/1F4LTfXn5KHR1LcNa5yXkkA   
auth code: 2jhd   
# Build quiche   
## Prerequisite  
```
apt-get install cmake build-essential protobuf-compiler libprotobuf-dev golang-go libunwind-dev libicu-dev  
```
## Build  
Download quiche library first. And I unzip the quiche.zip under "/home/xxx/"  
```
cd quiche  
mkdir build && cd build  
cmake ..  
make  
```
## Generate cert  
```
cd quiche/util  
chmod 777 /generate-certs.sh  
./generate-certs.sh   
mkdir -p data/quic-cert  
mv ./out/*  data/quic-cert/  
```
# Build quic module on ns3  
The code of quic module is tested on ns3.33.  
## Add environmental vairible
```
sudo gedit /etc/profile  
export QUICHE_SRC_DIR=/home/xxx/quiche/  
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$QUICHE_SRC_DIR/build/  
```
The reason to add QUICHE_SRC_DIR can be found quic/wscript.  
## build ns3  
1 cope quic folder to ns3.33/src/  
```
cd ns3.33  
source /etc/profile  
./waf configure  
./waf build  
```
## Run example 
1 put the file quic-main.cc(scratch) to ns3.33/scratch/  
2 set string varible quic_cert_path in quic-main.cc  
```
std::string quic_cert_path("/home/xxx/quiche/utils/data/quic-cert/");
```
3 Run example  
```
source /etc/profile  
./waf --run "scratch/quic-main"  
```

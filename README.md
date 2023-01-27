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
chmod 777 generate-certs.sh  
./generate-certs.sh   
mkdir -p data/quic-cert  
mv ./out/*  data/quic-cert/  
```
# Build quic module on ns3  
The code of quic module is tested on ns3.33.  
## Add environmental variable
```
sudo gedit /etc/profile  
export QUICHE_SRC_DIR=/home/xxx/quiche/  
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$QUICHE_SRC_DIR/build/  
```
The reason to add QUICHE_SRC_DIR can be found quic/wscript.  
## build ns3  
1 enble c++17 flag (ns3.33/wscript)
```
opt.add_option('--cxx-standard',  
               help=('Compile NS-3 with the given C++ standard'),  
               type='string', default='-std=c++17', dest='cxx_standard')  
```
2 cope quic folder to ns3.33/src/  
```
cd ns3.33  
source /etc/profile  
./waf configure  
./waf build  
```
# Run example 
1 put the file quic-main.cc(scratch) to ns3.33/scratch/  
2 set string varible quic_cert_path in quic-main.cc  
3 set string varible log_path in quic-main.cc to collect traced data.  
```
std::string quic_cert_path("/home/xxx/quiche/utils/data/quic-cert/");  
std::string log_path=std::string("/home/xxx/traces/")+algo+"/";  
```
3 Run example(BBR)  
```
source /etc/profile  
./waf --run "scratch/quic-main --cc=bbr"  
```
3 Run example(BBR)  
```
source /etc/profile  
./waf --run "scratch/quic-main --cc=cubic"  
```
# Results  
data can be found under folder ("/home/xxx/traces/")  
copy the file plot-script/data_plot.sh to "/home/xxx/traces/bbr"  
plot the results:  
```
chmod 777  data_plot.sh  
./data_plot.sh  
```
## bbr  
inflight packets:  
![avatar](https://github.com/SoonyangZhang/quic-on-ns3/blob/main/results/1-bbr-inflight.png)  
one way delay:  
![avatar](https://github.com/SoonyangZhang/quic-on-ns3/blob/main/results/1-bbr-owd.png)  
send rate:  
![avatar](https://github.com/SoonyangZhang/quic-on-ns3/blob/main/results/1-bbr-send-rate.png)  
## cubic  
inflight packets:  
![avatar](https://github.com/SoonyangZhang/quic-on-ns3/blob/main/results/1-cubic-inflight.png)  
one way delay:  
![avatar](https://github.com/SoonyangZhang/quic-on-ns3/blob/main/results/1-cubic-owd.png)  
send rate:  
![avatar](https://github.com/SoonyangZhang/quic-on-ns3/blob/main/results/1-cubic-send-rate.png)  
## copa
one way delay:  
![avatar](https://github.com/SoonyangZhang/quic-on-ns3/blob/main/results/1-copa-owd.png)  
goodput:  
![avatar](https://github.com/SoonyangZhang/quic-on-ns3/blob/main/results/1-copa-goodput.png)  

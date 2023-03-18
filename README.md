# quic-on-ns3
running google quic on ns3  
This repo depends on quiche library.   
The quiche library can be got here: https://github.com/bilibili/quiche  
I make some minor changes on CmakeList.txt to get shared library.  
Down quic library on release page: https://github.com/SoonyangZhang/quic-on-ns3/releases/tag/quiche  
  
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
cope quic folder to ns3.33/src/  
```
cd ns3.33  
CXXFLAGS="-std=c++17"  
source /etc/profile  
./waf configure  
./waf build  
```
# Run example 
1 put the file quic-main.cc(scratch) to ns3.33/scratch/  
2 Run example(BBR)  
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
chmod 777  data-plot.sh  
./data-plot.sh  
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
## vegas
one way delay:  
![avatar](https://github.com/SoonyangZhang/quic-on-ns3/blob/main/results/1-vegas-owd.png)  
goodput:  
![avatar](https://github.com/SoonyangZhang/quic-on-ns3/blob/main/results/1-vegas-goodput.png)  

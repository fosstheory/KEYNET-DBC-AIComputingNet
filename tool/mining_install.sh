#!/bin/bash

cpu_tensorflow_path="dbctraining/tensorflow-cpu-0.1.0:v1"
gpu_tensorflow_path="dbctraining/tensorflow-gpu-0.1.0:v1"
# add more images...

user=`whoami`
if [ $user == "root" ]; then
   echo "can not install mining_install.sh in root authorization"
   exit
fi


ubuntu_version=`lsb_release -r --short`
#if [ $ubuntu_version != "19.04" ] && [ $ubuntu_version != "18.04" ] && [ $ubuntu_version != "16.04" ] ; then
 #  echo "dismatch ubuntu19.04 or ubuntu18.04 or ubuntu16.04 version"
 #  exit
#fi
#echo "***check ubuntu version success***"

function version_lt() { test "$(echo "$@" | tr " " "\n" | sort -rV | head -n 1)" != "$1"; }
minimum_version="3.10.0"
kernel_version=`uname -r`
if version_lt $kernel_version $minimum_version; then
   echo "$kernel_version is less than $minimum_version"
   exit
fi
echo "***check kernel version success***"

echo y | sudo apt-get update
echo y | sudo apt-get -y install apt-transport-https ca-certificates curl software-properties-common
if [ $? -ne 0 ]; then
    echo "***install Dependent Libraries error***"
    exit
fi
echo "***install third library success***"

#curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -
cat ./docker_gpg.key | sudo apt-key add -
if [ $? -ne 0 ]; then
    echo "***curl apt-key error***"
    exit
fi
echo "***apt-key add success***"

sudo add-apt-repository \
       "deb [arch=amd64] https://download.docker.com/linux/ubuntu \
       $(lsb_release -cs) \
       stable"
#sudo apt-get update
#echo y | sudo apt-get -y install docker-ce=18.06.1~ce~3-0~ubuntu
sudo chmod 777 ./archive/containerd.io_1.2.12-1_amd64.deb  ./archive/docker-ce-cli_19.03.6~3-0~ubuntu-xenial_amd64.deb ./archive/docker-ce_19.03.6~3-0~ubuntu-xenial_amd64.deb
sudo dpkg -i   ./archive/containerd.io_1.2.12-1_amd64.deb  ./archive/docker-ce-cli_19.03.6~3-0~ubuntu-xenial_amd64.deb ./archive/docker-ce_19.03.6~3-0~ubuntu-xenial_amd64.deb
if [ $? -ne 0 ]; then
    echo "install docker-ce failed"
    exit
fi
echo "***install docker-ce success ***"

user_name=`whoami`
sudo gpasswd -a $user_name docker
if [ $? -eq 0 ]; then
    echo "add $user_name to docker success"
else
    echo "add docker user failed"
    exit
fi
echo "***add user to docker group success***"


set_docker_data_dir()
{
    echo "begin to configure the docker data dir"
    echo "below is your computer disk utilization:"
    echo -e
    df -hl | grep -v loop | grep -v tmpfs

    system_directory=`df -l | grep -v loop | grep -v tmpfs | sort -n -r -k 4 |awk '{print $6}'`


    array[0]="/var/lib/docker"

    a=1
    for line in $system_directory
    do
       array[$a]=$line
       a=$(($a+1))
    done
    echo -e

    default_install_directory=`df -l | grep -v loop | grep -v tmpfs | sort -n -r -k 4 |awk 'NR==1{print}'|awk '{print $6}'`

    echo "Please choose your host_volum_dir directory,eg.you can input 0 if you want to set host_volum_dir as ${array[0]},recommend to choose the directory which has Maximal remaining space"
    echo "***NOTE:if you ENTER directly the host_volum_dir default value is $default_install_directory (Maximal remaining space)"
    echo "The directory below has already been descending order by remaining space "
    echo -e

    length=$((${#array[@]}-1))
    loop=$(($length-1))

    for(( i=0;i<$length;i++)) do
        echo [$i]:${array[$i]}
    done
    echo -e

    read -p "Please input the number from 0 to $loop,can also input ENTER directly to use the default value:" num
    while [[ $num -lt 0 || $num -gt $loop ]];do
       read -p "you have input a invalid number,it must be from 0 to $loop,please reinput:" num
    done

    if [ -z $num ];then
        t1=$(echo $default_install_directory | sed 's/\/$//')
    else
        t1=$(echo ${array[$num]} | sed 's/\/$//')
    fi

    if [ 0 -eq $num ]; then
        docker_install_dir="$t1"
    else
        docker_install_dir="$t1/docker_data"
    fi
}

set_docker_data_dir

#sudo echo 'DOCKER_OPTS="-H unix:///var/run/docker.sock -H tcp://127.0.0.1:31107"' >> /etc/default/docker
sudo sed -i "s;ExecStart=.*;ExecStart=/usr/bin/dockerd -H fd:// -H tcp://127.0.0.1:31107 -H unix:///var/run/docker.sock --data-root=\"$docker_install_dir\" --add-runtime=nvidia=/usr/bin/nvidia-container-runtime --live-restore;g" /lib/systemd/system/docker.service
sudo systemctl stop docker
if [ $? -ne 0 ]; then
    exit
fi
sudo systemctl daemon-reload
if [ $? -ne 0 ]; then
    exit
fi
sudo systemctl start docker
if [ $? -ne 0 ]; then
    exit
fi

#newgrp - dockerc

gpu_flag=`lspci |grep -i nvidia`
if [ $? -eq 0 ]; then
    echo "install nvidia docker"

# jimmy: install nvidia-docker 1 from public repo
#    wget -P /tmp https://github.com/NVIDIA/nvidia-docker/releases/download/v1.0.1/nvidia-docker_1.0.1-1_amd64.deb
#    if [ $? -ne 0 ]; then
#        echo "***wget nvidia-docker deb failed***"
#        exit
#    fi
#    echo "***wget nvidia-docker deb success ***"
#    sudo dpkg -i /tmp/nvidia-docker*.deb && rm /tmp/nvidia-docker*.deb
#    if [ $? -ne 0 ]; then
#        echo "***dpkg nvidia-docker deb failed***"
#        exit
#    fi
#    echo "***install nvidia-docker success ***"

# jimmy: install nvidia-docker 2 from public repo
#    curl -s -L https://nvidia.github.io/nvidia-docker/gpgkey | sudo apt-key add -
#    distribution=$(. /etc/os-release;echo $ID$VERSION_ID)
#    curl -s -L https://nvidia.github.io/nvidia-docker/$distribution/nvidia-docker.list | sudo tee /etc/apt/sources.list.d/nvidia-docker.list
#    sudo apt-get -y update
#    sudo apt-get -y install nvidia-docker2=2.0.3+docker18.06.1-1 nvidia-container-runtime=2.0.0+docker18.06.1-1
#    sudo pkill -SIGHUP dockerd


    # jimmy: install nvidia-docker 2 from local archive
    sudo chmod 777  ./archive/libnvidia-container1_1.0.7-1_amd64.deb ./archive/libnvidia-container-tools_1.0.7-1_amd64.deb
    sudo chmod 777  ./archive/nvidia-container-toolkit_1.0.5-1_amd64.deb ./archive/nvidia-container-runtime_3.1.4-1_amd64.deb
    sudo dpkg -i ./archive/libnvidia-container1_1.0.7-1_amd64.deb ./archive/libnvidia-container-tools_1.0.7-1_amd64.deb
    sudo dpkg -i ./archive/nvidia-container-toolkit_1.0.5-1_amd64.deb ./archive/nvidia-container-runtime_3.1.4-1_amd64.deb
    if [ $? -ne 0 ]; then
        echo "***dpkg nvidia-docker deb failed***"
        exit
    fi
    echo "***install nvidia-docker success ***"
    sudo pkill -SIGHUP dockerd
fi

#gpu_flag=`lspci |grep -i nvidia`
#if [ $? -eq 0 ]; then
#    echo "need to download GPU-related tensorflow"
#    sudo docker pull $gpu_tensorflow_path
#    if [ $? -eq 1 ]; then
#        echo "pull fail"
#	exit
#    fi
#    echo "***pull tensorflow GPU image success***"
#else
#    echo "need to download CPU-related tensorflow"
#    sudo docker pull $cpu_tensorflow_path
#    if [ $? -eq 1 ]; then
#        echo "pull fail"
#	exit
#    fi
#    echo "***pull tensorflow CPU image success***"
#fi

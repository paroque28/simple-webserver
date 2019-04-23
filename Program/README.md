# Proyecto 1 Sistemas Operativos

## Documentation
[Access here the documentation](doc/documentation.md)

## Automatic Installation
```bash
sh install_docker.sh
```

## Manual Installation (Skip)

### Ubuntu
First install docker-ce:

#### Install repository
```bash
sudo apt-get update
sudo apt-get install \
    apt-transport-https \
    ca-certificates \
    curl \
    gnupg-agent \
    software-properties-common
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -
sudo add-apt-repository \
   "deb [arch=amd64] https://download.docker.com/linux/ubuntu \
   $(lsb_release -cs) \
   stable"
```
#### Install docker-ce
```bash
sudo apt-get install docker-ce docker-ce-cli containerd.io
```
#### Install docker-compose
```bash
sudo apt-get install docker-compose
```

## Usage
To run all webservers:
```bash
    docker-compose up -d
```
To stop all webservers:
```bash
    docker-compose down
```
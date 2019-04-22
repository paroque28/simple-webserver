FROM paroque28/centos-systemd-gcc
ARG TYPE=FIFO
##########################
# # Old image without GCC
# FROM centos/systemd 
# # Initial install tools and change password
# RUN yum clean all
# RUN rpm --rebuilddb
# RUN yum -y update
# RUN yum -y install make gcc
# RUN echo "root" | passwd --stdin root 
##############################

#Copy files to container
COPY src/ /app/src
COPY Makefile /app

RUN make -C /app TYPE=${TYPE}
RUN mv /app/webserver /usr/bin
# Create Initial configuration
RUN mkdir -p /etc/webserver
COPY webserver-default.conf /etc/webserver/config.conf
# Create Daemon
COPY WebServer.service /etc/systemd/system
# Run un boot
RUN ln -s /etc/systemd/system/WebServer.service /etc/systemd/system/multi-user.target.wants/WebServer.service

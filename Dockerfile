FROM centos/systemd

# Install make
RUN yum clean all
RUN rpm --rebuilddb
RUN yum -y update
RUN yum -y install make gcc
RUN echo "root" | passwd --stdin root 

COPY . /app
RUN make -C /app
# Create Initial configuration
RUN mkdir -p /etc/webserver
RUN cp /app/webserver.conf /etc/webserver/config.conf
# Create Daemon
RUN cp /app/webserver.service /etc/systemd/system
# Run un boot
RUN ln -s /etc/systemd/system/webserver.service /etc/systemd/system/multi-user.target.wants/webserver.service

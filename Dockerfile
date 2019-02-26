FROM centos/systemd

# Install make
RUN yum -y update
RUN yum -y install make
RUN echo "root" | passwd --stdin root 

COPY . /app
RUN make -C /app
# Create Initial configuration
RUN mkdir -p /etc/webserver
RUN cp /app/simple-server.conf /etc/webserver/config.conf
# Create Daemon
RUN cp /app/simple-server.service /etc/systemd/system
# Run un boot
RUN ln -s /etc/systemd/system/simple-server.service /etc/systemd/system/multi-user.target.wants/simple-server.service

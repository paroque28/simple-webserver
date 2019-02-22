FROM centos/systemd

# Install make
RUN yum -y update
RUN yum -y install make
RUN echo "root" | passwd --stdin root 

COPY . /app
RUN make -C /app
RUN cp /app/simple-server.service /etc/systemd/system
RUN ln -s /etc/systemd/system/simple-server.service /etc/systemd/system/multi-user.target.wants/simple-server.service
#CMD 

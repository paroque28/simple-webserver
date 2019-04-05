
# bclient <machine> <port> <file> <N-threads> <N-cycles>
• Where <machine> indicates the name or IP address where the server is running
• <port> is the port number of the corresponding server
• <file> is the path to the file that is going to be transferred (it could be a text file or an image
of any size)
• The client program is going to create <N-threads> that are going to send requests to the
corresponding server. Each thread is going to repeat this request <N-cycles> times.
There are some warnings that need to be solved
Run execution command example

./bclient localhost 8080 test.jpg 1 1 
./bclient localhost 8080 test.iso 1 1 

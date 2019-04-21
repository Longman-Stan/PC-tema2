build: server subscriber udpclient

server:
	g++ server.cpp -o server

subscriber:
	g++ subscriber.cpp -o subscriber

udpclient:
	g++ udpclient.cpp -o udp_client

clean:
	rm server subscriber udp_client

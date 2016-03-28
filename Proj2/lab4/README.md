#NS-Chain

##Authors

Nathan Turner

Jason Song

##Usage

	cp udpchain.cc /scratch/udpchain.cc
	./waf
	./waf --run "scratch/udpchain true"

##Description

Code creates 4 nodes with 3 point connections between nodes A and B, B and C, and C and D. Each connection is on a different network.

Client node A sends 5 packets to server D.

The bool attached to the run command tells the program wheather or not to log data.

##Log Info

Each node creates a pcap log which can be opened with "/usr/sbin/tcpdump -tt -nn -r filename"


#NS-Chain

##Authors

Nathan Turner

Jason Song

##Usage

	cp tcpchain.cc /scratch/tcpchain.cc
	./waf
	./waf --run "scratch/tcpchain --errorRate=.00001 --outputCwnd=tcp.cwnd --verbose=true"
	./waf --run "scratch/tcpchain --errorRate=.00005 --outputCwnd=tcpWorse.cwnd --verbose=true"
	gnuplot gnuplot

##Description

Code creates 4 nodes with 3 point connections between nodes A and B, B and C, and C and D. Each connection is on a different network.

Attaches application on client node to log cwnd size over time and writes data to tcpchain.cwnd

Client node A sends 5 packets to server D.

The bool attached to the run command tells the program wheather or not to log data.

##Log Info

Each node creates a pcap log which can be opened with "/usr/sbin/tcpdump -tt -nn -r tcpchain.pcap"

cwnd log info can be found in tcpchain.cwnd

##Graph cwnd

gnuplot contains commands to graph cwnd data in one file called cwnd.png and can be called with

gnuplot gnuplot



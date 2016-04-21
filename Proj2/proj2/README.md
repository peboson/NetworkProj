#Project 2

##Authors

Nathan Turner

Jason Song

#Usage

*** NO NEED TO MODIFY NS3 SRC ***

    cp -r ./* $NS3ROOT/
    cd $NS3ROOT
    ./proj2p1.sh
    ./proj2p2.sh
    ./proj2p3.sh

##Description

Code creates 4 nodes with 3 point connections between nodes A and B, B and C, and C and D. 
Each connection is on a different network.

Attaches application on client node to log cwnd size over time and writes data to tcpchain.cwnd

Client node A sends 1MB to server D using tcp-large-transfer.cc example as model.

Bandwidth of 1Mbps and delay of 10ms with bit error rate of 10^-6 between A-B and C-D and 
10^-5 between B-c.

Command line arguments are in script. "errorRate" changes the BER between B-C, "outputCwnd"
is the file to ouput the cwnd data, "verbose" is a bool to turn on logging, "SocketType" describes
whether to use "TcpNewReno" or "TcpFixed", and "InitialCwnd" sets the number of packets initialized
for the congestion window.
 
##Log Info

After running proj2px.sh, the pcap is stored in "tcpchain-x-x.pcap", the text trace is stored in
"*.tcpdump", the sequence vs time data is stored in "*.data", the congestion windows data is stored
in "tcp.cwnd"

##Graph cwnd

Graph of seq number vs time with slow start in a different collor can be found in proj2p1.png

Graph of cwnd vs time can be found in proj2p2.png

Graphs of cwnd vs time and seq number vs time for NewReno and TcpFixed can be found in proj2p3-cwnd.png
and proj2p3-seq.png respectively

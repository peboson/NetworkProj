echo "Executing tcpchain ..."
echo "./waf --run 'tcpchain --SocketType=TcpNewReno --InitialCwnd=1'"
./waf --run "tcpchain --SocketType=TcpNewReno --InitialCwnd=1"

echo "Executing tcpdump > proj2p1.tcpdump ..."
/usr/sbin/tcpdump -tt -vv -r tcpchain-0-0.pcap |tee proj2p1.tcpdump

echo "Executing awk > proj2p1.data ..."
./SeqVsTime.awk proj2p1.tcpdump |tee proj2p1.data

echo "Executing gnuplot > proj2p1.png ..."
gnuplot proj2p1.gnuplot


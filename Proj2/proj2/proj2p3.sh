echo "Executing tcpchain with TcpNewReno ..."
echo "./waf --run 'tcpchain --SocketType=TcpNewReno --InitialCwnd=1'"
./waf --run "tcpchain --SocketType=TcpNewReno --InitialCwnd=1"

echo "Executing tcpdump > proj2p3-newreno.tcpdump ..."
/usr/sbin/tcpdump -tt -vv -r tcpchain-0-0.pcap |tee proj2p3-newreno.tcpdump

echo "Executing awk > proj2p3-newreno.data ..."
./SeqVsTime.awk proj2p3-newreno.tcpdump |tee proj2p3-newreno.data

echo "Executing Cwnd > proj2p3-newreno.cwnd"
cp tcp.cwnd proj2p3-newreno.cwnd

echo "Executing tcpchain with TcpFixed ..."
echo "./waf --run 'tcpchain --SocketType=TcpFixed --InitialCwnd=100'"
./waf --run "tcpchain --SocketType=TcpFixed --InitialCwnd=100"

echo "Executing tcpdump > proj2p3-fixed.tcpdump ..."
/usr/sbin/tcpdump -tt -vv -r tcpchain-0-0.pcap |tee proj2p3-fixed.tcpdump

echo "Executing awk > proj2p3-fixed.data ..."
./SeqVsTime.awk proj2p3-fixed.tcpdump |tee proj2p3-fixed.data

echo "Executing Cwnd > proj2p3-fixed.cwnd"
cp tcp.cwnd proj2p3-fixed.cwnd

echo "Executing gnuplot > proj2p3.png ..."
gnuplot proj2p3.gnuplot


echo "Executing tcpchain ..."
echo "./waf --run 'tcpchain --SocketType=TcpNewReno --InitialCwnd=1'"
./waf --run "tcpchain --SocketType=TcpNewReno --InitialCwnd=1"

echo "Executing Cwnd > proj2p2.cwnd"
cp tcp.cwnd proj2p2.cwnd

echo "Executing gnuplot > proj2p2.png ..."
gnuplot proj2p2.gnuplot


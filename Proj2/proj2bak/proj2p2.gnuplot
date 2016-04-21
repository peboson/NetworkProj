set terminal png
set output "cwnd.png"
plot "tcp.cwnd" using 1:2 with points, "tcp.cwnd" using 1:3 with points

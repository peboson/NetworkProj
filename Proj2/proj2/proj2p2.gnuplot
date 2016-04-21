set terminal png
set output "proj2p2.png"
plot "proj2p2.cwnd" using 1:2 with points, "proj2p2.cwnd" using 1:3 with points

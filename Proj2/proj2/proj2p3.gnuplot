set terminal png
set output "proj2p3-seq.png"
plot "proj2p3-newreno.data" using 1:2 with linespoints, "proj2p3-fixed.data" using 1:2 with linespoints, "proj2p3-newreno.data" using 1:3 with points, "proj2p3-fixed.data" using 1:3 with points
set terminal png
set output "proj2p3-cwnd.png"
plot "proj2p3-newreno.cwnd" using 1:2 with points, "proj2p3-fixed.cwnd" using 1:2 with points, "proj2p3-newreno.cwnd" using 1:3 with points, "proj2p3-fixed.cwnd" using 1:3 with points

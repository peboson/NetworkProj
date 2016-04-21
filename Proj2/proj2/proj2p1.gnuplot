set terminal png
set output "proj2p1.png"
plot "proj2p1.data" using 1:2 with linespoints, "proj2p1.data" using 1:3 with points

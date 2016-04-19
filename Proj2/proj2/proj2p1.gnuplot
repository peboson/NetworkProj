set terminal png
set output "cwnd.png"
plot "send.cwnd" with linespoints, "resend.cwnd" with points 

#!/bin/awk -f
BEGIN {
#	print "File";
	seqmax=0;
	time="";
}
NR%2==1 {
#	print $1;
	time=$1;
}
NR%2==0 {
#	print $12;
	if($12!="1,"){
		seqstr=$12;
		if(index(seqstr,",")>0){
			seqstr=substr(seqstr,0,index(seqstr,","));
		}
		if(index(seqstr,":")>0){
			seqstr=substr(seqstr,0,index(seqstr,":"));
		}
		seq=strtonum(seqstr);
		if(seq>seqmax){
			seqmax=seq;
			print time, "\t", seq > "send.cwnd";
		}else if(seq>0){
			print time, "\t", seq > "resend.cwnd";
		}
	}
}
END {
#	print "Done";
}

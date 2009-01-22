echo "interval,avg,max,time"
if [[ "$2" == "" ]]; then 
  rm -f filein.log filein_sort.log
  egrep "PVMFSocketNode::PortActivity:.*type=5" $1 | sed -e "s/.*:Time=//" | sed -e "s/:.*//" > filein.log ; cat filein.log | sort -n > filein_sort.log
fi
echo "file in,"
perl gt.pl filein_sort.log

if [[ "$2" == "" ]]; then 
rm -f sockin.log sockin_sort.log
grep "PVMFSocketNode::ProcessIncomingMsg: aP" $1 | sed -e "s/.*:Time=//" | sed -e "s/:.*//" > sockin.log ; cat sockin.log | sort -n > sockin_sort.log
fi
echo "socket in,"
perl gt.pl sockin.log

if [[ "$2" == "" ]]; then 
rm -f socksend.log socksend_sort.log
grep "PVMFSocketNode::HandleSocketEvent() In aId=1, aFxn=1" $1 | sed -e "s/.*:Time=//" | sed -e "s/:.*//" > socksend.log ; cat socksend.log | sort -n | uniq > socksend_sort.log
fi
echo "socket send,"
perl gt.pl socksend_sort.log

if [[ "$2" == "" ]]; then 
rm -f sockrecv.log sockrecv_sort.log
grep "45 bytes" $1 | sed -e "s/.*:Time=//" | sed -e "s/:.*//" > sockrecv.log ; cat sockrecv.log | sort -n | uniq > sockrecv_sort.log
fi
echo "socket recv,"
perl gt.pl sockrecv.log

if [[ "$2" == "" ]]; then 
rm -f sockout.log sockout_sort.log
grep "PVMFSocketNode::ProcessOutgoingMsg: aP" $1 | sed -e "s/.*:Time=//" | sed -e "s/:.*//" > sockout.log ; cat sockout.log | sort -n > sockout_sort.log
fi
echo "socket out,"
perl gt.pl sockout.log

if [[ "$2" == "" ]]; then 
rm -f jbin.log jbin_sort.log
grep "PVMFJitterBufferNode::ProcessIncomingMsg: aP" $1 | sed -e "s/.*:Time=//" | sed -e "s/:.*//" > jbin.log ; cat jbin.log | sort -n > jbin_sort.log
fi
echo "jb in,"
perl gt.pl jbin.log

if [[ "$2" == "" ]]; then 
rm -f ap.log  ap_sort.log
grep "PVMFJitterBufferImpl::addPacket: M" $1 | sed -e "s/.*:Time=//" | sed -e "s/:.*//" > ap.log ; cat ap.log | sort -n > ap_sort.log
fi
echo "add packet,"
perl gt.pl ap.log

if [[ "$2" == "" ]]; then 
rm -f jbout.log jbout_sort.log
grep "PVMFJitterBufferNode::ProcessOutgoingMsg: aP" $1 | sed -e "s/.*:Time=//" | sed -e "s/:.*//" > jbout.log ; cat jbout.log | sort -n > jbout_sort.log
fi
echo "jb out,"
perl gt.pl jbout.log

if [[ "$2" == "" ]]; then 
rm -f mlin.log mlin_sort.log
grep "PVMFMediaLayerNode::ProcessIncomingMsg: aP" $1 | sed -e "s/.*:Time=//" | sed -e "s/:.*//" > mlin.log ; cat mlin.log | sort -n > mlin_sort.log
fi
echo "ml in,"
perl gt.pl mlin.log

if [[ "$2" == "" ]]; then 
rm -f mlout.log mlout_sort.log
grep "PVMFMediaLayerNode::ProcessOutgoingMsg: aP" $1 | sed -e "s/.*:Time=//" | sed -e "s/:.*//" > mlout.log ; cat mlout.log | sort -n > mlout_sort.log
fi
echo "ml out,"
perl gt.pl mlout.log

if [[ "$2" == "" ]]; then 
rm -f fileout.log fileout_sort.log
egrep "PVMFFileOutputNode::PortActivity:.*type=5" $1 | sed -e "s/.*:Time=//" | sed -e "s/:.*//" > fileout.log ; cat fileout.log | sort -n > fileout_sort.log
fi
echo "file out,"
perl gt.pl fileout.log





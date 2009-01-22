#
# Player smoke test
#

echo "Setting lib path"
export LD_LIBRARY_PATH=$CFG_DIR/build/installed_lib/linux
echo "Lib Path" $LD_LIBRARY_PATH

echo "Removing old output"
rm -f smoke*.txt

cd $CFG_DIR/build/bin/linux

echo "Fetching config files"
rm -f *.cfg
cp $SRC_ROOT/tools_v2/build/package/opencore/*.cfg ./
echo "Running player tests"

echo "Copying test content"
cp $SRC_ROOT/engines/player/test/data/test_m4v_amr.mp4 .
cp $SRC_ROOT/engines/player/test/data/pv_amr_mpeg4.sdp .

echo "MP4 Local..."
./pvplayer_engine_test -output $CFG_DIR/smoke_local_mp4.txt -logfile -test 51 51

echo "MP4 Download..."
./pvplayer_engine_test -output $CFG_DIR/smoke_dl_mp4.txt -logfile -test 102 106 -source http://pvwmsoha.pv.com:7070/MediaDownloadContent/MP4/prog_dl/mpeg4+aac_metadata_qt.mp4

echo "RTSP Streaming..."
./pvplayer_engine_test -output $CFG_DIR/smoke_sm_rtsp.txt -logfile -test 851 851 

echo "Done!"
cd $CFG_DIR

grep "Successes: " smoke*.txt
grep "Failures: " smoke*.txt
grep "Memory Leaks" smoke*.txt

#END OF SCRIPT, NO TEXT BEYOND THIS LINE

open (INPUT, $ARGV[0]);

my $lasttime=0;
my $numintervals=0;
my $total_interval=0;
my $max_interval=0;
my $max_interval_timeval;

while (<INPUT>) {
  chomp;
  my $timeval = $_;
  if ($numintervals++ != 0) {
    $total_interval += ($timeval-$lasttime);
    if (($timeval-$lasttime) > $max_interval) {
       $max_interval = $timeval-$lasttime;
       $max_interval_timeval = $timeval; 
    }
  }
  $lasttime = $timeval;
}

$avg = $total_interval/$numintervals;
print "$avg,$max_interval,$max_interval_timeval\n";

close INPUT;


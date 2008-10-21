Use perl
  eval 'exec perl -S $0 "$@"'
  if 0;

use Getopt::Std;

getopts('r');                   

my $file = $ARGV[0];
my $vlabel;

sub check_ccspec {

  if (! -f ".ccspec") {
    return 1;
  }

  unlink(".ccspec.new");
  system("cleartool catcs > .ccspec.new");

  my $cmp = `/extern_tools/bin/win32/cmp .ccspec .ccspec.new`;  # dos: comp, unix: cmp
  if ($cmp) {
    unlink(".ccspec.new");
    return 1;
  }
  unlink(".ccspec.new");
  return 0;
}


sub compare_mtime {
  my $file1 = shift @_;
  my $file2 = shift @_;

 
  if (! -f $file1 && ! -f $file2) {
    return 0;
  }

  if (! -f $file2) {
    return 1;
  }
  
  if (! -f $file1) {
    return -1;
  }

  my @f1_stat = stat($file1);
  my @f2_stat = stat($file2);

  if ($f1_stat[9] < $f2_stat[9]) {
    return -1;
  }
  elsif ($f1_stat[9] > $f2_stat[9]) {
    return 1;
  }
  
  return 0;
}
sub usage {
  print "must provide <output_fname> [prod_id]\n";
  print "default prod_id is PVSS\n";
}


if ($#ARGV < 1) {
  usage();
}
my $status = check_ccspec();
if (! -f $file) {
   $status = 1;
}
if (!$status && -f "VERSION" && -f $file) {
  if (compare_mtime($file, "VERSION") <= 0) {
    # VERSION is newer
    $status = 1;
  }
}

if (! $status) {
   exit(0);
}

# remove the output file first 
unlink($file);

if (-f "VERSION") {

  # check if VERSION is newer than .ccspec
  if (compare_mtime("VERSION", ".ccspec") <= 0) {
    # config spec is newer

    if (check_ccspec()) {
      # see if config spec has changed
      die "*** ERROR -- Config spec has changed -- must update VERSION file ***\n";
    }
  }
  my $label;

  open(VFILE, "VERSION") || die "*** ERROR -- Can't open file VERSION ***\n";
  while (<VFILE>) {
    chomp;
    s/#.*//;	      # remove from comment marker to end-of-line.
    next if ( /^\s*$/);   # skip blank lines

    if (($label) = m/Version:\s*(.*)/i) {
      last;
    }
  }

  if ($label) {
    $vlabel = $label;
  }

  close (VFILE);

  if (!$vlabel) {
    die "*** ERROR -- No valid label found in VERSION file ***\n";
  }

  # save .ccspec 
  unlink(".ccspec");
  system("cleartool catcs > .ccspec");

} else {

  print "No VERSION file, so check ccspec\n";
  # save .ccspec 
  unlink(".ccspec");
  system("cleartool catcs > .ccspec");

  my (@line, $tmp);
  open(CCLABEL, "cleartool ls -d . |") || die "*** ERROR -- couldn't run cleartool ***\n";
OUTER:  while (<CCLABEL>) {
    chomp;
    @line = split(' ', $_);
    while ($tmp = shift(@line)) {
      print "$tmp\n";
      if ($tmp eq "Rule:") {
        $vlabel = shift(@line);
        last OUTER;
      }
    }
  }

  close(CCLABEL);

  # split the path to get the last element
  @line = split('/', $vlabel);
  # print "line = @line\n";
  $vlabel = $line[$#line];
  
  print "vlabel is $vlabel\n";

  my $proc_vlabel;
  if (($proc_vlabel) = ($vlabel =~ m/([0-9]+[.][0-9]+.*)/i)) {
    # $proc_vlabel =~ s/_/./g;
    $vlabel = $proc_vlabel;
  }
  elsif ($opt_r) {
    die "*** ERROR -- No valid label found ***\n";
  } else {
    $vlabel = "3.1_default";
  }

}

$prod_id = "PVSS"; 
if  (defined($ARGV[1])) {
  $prod_id =  $ARGV[1];
  printf "argv1 is $ARGV[1], prod_id is $prod_id\n";
}


open(TFILE, "> $file");
binmode(TFILE);
print TFILE "char   Version[]      = \"$prod_id/$vlabel (\"__TIME__\" \"__DATE__\")\";\n";
print TFILE "char   SM_UserAgent[] = \"PVServer-SM/$vlabel\";\n";
close(TFILE);

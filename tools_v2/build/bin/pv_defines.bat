@rem = 'Perl, ccperl read this as an array assignment & skip the goto
@echo off
goto endofperl
@rem ';
#!/usr/atria/bin/Perl

######################################################################

use Getopt::Long;


{
    local(@whoami) = split('/', $0);
    $whoami = pop(@whoami);
};

sub determine_os {
  my $os;
  my $windows_os=`ver`;
  my $define;

  if ( -x "/bin/uname" ) {
    $os = `/bin/uname -s`;
  }
  elsif ( -x "/bin/mach" ) {
    $os=`/bin/mach`;
  }
  elsif ( -x "/usr/ucb/mach" ) {
    $os=`/usr/ucb/mach`;
  }
  elsif ( $windows_os =~ m/Windows/ ) {
    $os=$windows_os;
  }
  else {
    $os="unknown";
  }
  
  # map to correct define value
  if ($os =~ m/Linux/ ) {
    $define = "-DPV_OS_LINUX";
  }
  elsif ($os =~ m/SunOS/ ) {
    $define = "-DPV_OS_SOLARIS";
  }
  elsif ($os =~ m/HP-UX/ ) {
    $define = "-DPV_OS_HPUX";
  }
  elsif ($os =~ m/Microsoft Windows/ ) {
    if ($build_arch =~ m/win32/ ) {
      $define = "/D \"PV_OS_WINDOWS_NT\"";
    }
    elsif ($build_arch =~ m/wince/ ) {
      $define = "";
    }
    else {
      $define = "-DPV_OS_WINDOWS_NT";
    } 
  }
  else {
    $define = "-DPV_OS_UNKNOWN";
  }
  return $define;
}

sub determine_cpu {
  my $model;
  my $define;

  # The backtick operator doesn't seem to work with
  # this utility, so redirect the output to a temporary file
  if ((defined ($tmp = $ENV{"TMP"})) && ($build_arch =~ m/win32/ )) {
     # Usually this is only defined in the DOS shell
     system("/tools/build/bin/chkcpu > $tmp/chkcpu");
     open(CPUCHK, "$tmp/chkcpu");
     while ($_ = <CPUCHK>) {
        if (/CPU Vendor and Model/) {
          $model = $_;
        }
     }
     close CPUCHK;
     unlink "$tmp/chkcpu";
  }
  elsif ( -f "/proc/cpuinfo" ) {
    open(FH, "/proc/cpuinfo");
    while ($_ = <FH>) {
      if (/model name/) {
        $model = $_;
      }
    }
    close FH;
  }
  elsif ( -x "/bin/uname" ) {
    $model = `/bin/uname -p`;
  }
  else {
    $model ="unknown";
  }

  # map to correct define value
  if ($model =~ m/Pentium III|PentiumIII|Celeron/ ) {
    if ($build_arch =~ m/win32/ ) {
      $define = "/D \"PV_PROCESSOR_PENTIUM_3\""
    }
    else {
      $define = "-DPV_PROCESSOR_PENTIUM_3";
    }
  }
  elsif($model =~ m/Pentium II|PentiumII/) {
    if ($build_arch =~ m/win32/ ) {
      $define = "/D \"PV_PROCESSOR_PENTIUM_2\""
    }
    else {
      $define = "-DPV_PROCESSOR_PENTIUM_2";
    }
  }
  elsif($model =~ m/sparc/) {
    $define = "-DPV_PROCESSOR_SPARC";
  }
  else {
    $define = "-DPV_PROCESSOR_UNKNOWN";
  }
  return $define;
}


my %cmdline_opts = ();
my $all = 1;
my $output_str;
my $os_type;
my $cpu_type;

GetOptions(\%cmdline_opts, "os", "cpu");

if (defined($cmdline_opts{os})) {
  $all = 0;
}

if (defined($cmdline_opts{cpu})) {
  $all = 0;
}

$build_arch = $ARGV[0];

if ($all || defined($cmdline_opts{os})) {
  # set the OS variable

  if ($build_arch =~ m/mocha/i) {
    $os_type = '-DPV_OS_MOCHA';    
  }
  else { 
    # Assume OS for build target is same as current host
    $os_type = determine_os();
  }
  $output_str .= $os_type . " ";
}

if ($all || defined($cmdline_opts{cpu})) {
  # set the CPU variable

  if ($os_type eq '-DPV_OS_HPUX') {
    $cpu_type = '-DPV_PROCESSOR_HP_9000';
  }
  elsif ($build_arch =~ m/win32/i) {
    if ($build_arch =~ m/_arm/i) {
      $cpu_type = '-DPV_PROCESSOR_ARM';
    }
  }
  elsif ($build_arch =~ m/wince/i) {
    if ($build_arch =~ m/_arm/i) {
      # Define PocketPC for MS Embedded SDK
      $cpu_type = '/D "ARM" /D "_ARM_" /D "_MT" /D "WIN32_PLATFORM_PSPC"';
    }
  }
  else {
    # Assume processor for build target is same as current host
    $cpu_type = determine_cpu();
  }
  $output_str .= $cpu_type . " ";
}

print "$output_str";

exit(0);

__END__

:endofperl
ccperl -e "$s = shift; $c = $s =~ /.bat$/ ? $s : $s.'.bat'; $p = (-x $c) ? '' :' S '; system('ccperl '.$p.$c.' '.join(' ',@ARGV)); exit $?;" %0 %1 %2 %3 %4 %5 % 6 %7 %8 %9

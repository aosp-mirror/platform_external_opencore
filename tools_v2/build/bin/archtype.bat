@rem = 'Perl, ccperl read this as an array assignment & skip the goto
@echo off
goto endofperl
@rem ';
#!/usr/atria/bin/Perl

######################################################################

my $mach = 'unknown';

if ($ENV{'OS'} =~ m/Windows/) {
   printf("win32\n");
   exit(0);
}

if ( -x '/bin/mach' ) {
   $mach = `/bin/mach`;
} elsif ( -x '/usr/ucb/mach' ) {
   $mach = `/usr/ucb/mach`;
} elsif ( -x '/bin/uname' ) {
   $mach = `/bin/uname -s`;
} 
chomp $mach;

#print "mach = $mach\n";

my $archtype=$mach;
if ($mach eq 'sparc') {
  $archtype = 'sun';
}
elsif ( $mach eq 'HP-UX' ) {
  $archtype='hpux';
}
elsif ( $mach eq 'Linux' ) {
  my $status = `/sbin/ldconfig -p | /bin/grep -q libc.so.6`;
  if (! $?) {
    $archtype='linux';
  }
  else {
    $archtype='linux5';
  }
}

print "$archtype\n";

exit(0);

__END__

:endofperl
ccperl -e "$s = shift; $c = $s =~ /.bat$/ ? $s : $s.'.bat'; $p = (-x $c) ? '' :' S '; system('ccperl '.$p.$c.' '.join(' ',@ARGV)); exit $?;" %0 %1 %2 %3 %4 %5 %6 %7 %8 %9

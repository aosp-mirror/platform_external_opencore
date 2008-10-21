@rem = 'Perl, ccperl read this as an array assignment & skip the goto
@echo off
goto endofperl
@rem ';
#!/usr/atria/bin/Perl

######################################################################

use Getopt::Long;

# This is meant to be a win32 replacement for the Unix install utility

# The options -c and -m will be accepted for compatibility, but ignored

GetOptions('c' => \$overwrite, 'm=i' => \$mode);

sub Copy {
  my ($from, $to) = @_;

  if ( ! open(FROM, "$from") ) {
    return 0;
  }       

  if ( -d $to ) {
    if ( ! open(TO, ">$to/$from")) {
      close FROM;
      return 0;
    }
  }
  else {
    if ( ! open(TO, ">$to")) {
      close FROM;
      return 0;
    }
  }

  while (<FROM>) {
    print TO "$_";
  }

  close TO;
  close FROM;
  return 1;
}

if ( ! defined $ARGV[0] ) {
  print STDERR "cc_install: must specify source\n";
  exit (1);
}
if ( ! defined $ARGV[1] ) {
  print STDERR "cc_install: must specify destination\n";
  exit (1);
}

my $source=$ARGV[0];
my $destination=$ARGV[1];

#Copy ($source, $destination);

# Convert to DOS path delimeters
$source =~ s/\//\\/g;
$destination =~ s/\//\\/g;

$ret=system("COPY /Y $source $destination");
if ($?) {
  print STDERR "Can not copy $source to $destination: $!\n";
  exit (1);
}

exit(0);

__END__

:endofperl
ccperl -e "$s = shift; $c = $s =~ /.bat$/ ? $s : $s.'.bat'; $p = (-x $c) ? '' :' S '; system('ccperl '.$p.$c.' '.join(' ',@ARGV)); exit $?;" %0 %1 %2 %3 %4 %5 %6 %7 %8 %9

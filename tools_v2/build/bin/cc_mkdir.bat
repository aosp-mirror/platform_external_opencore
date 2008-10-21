@rem = 'Perl, ccperl read this as an array assignment & skip the goto
@echo off
goto endofperl
@rem ';
#!/usr/atria/bin/Perl

######################################################################

my $path = $ARGV[0];

if ($path eq "") {
	print STDERR "Must specify directory/path to create\n";
	exit (-1);
}

if ($ENV{'OS'} =~ m/Windows/) {
	# Convert any DOS backslashes to Unix forward slashes
	$path =~ s/\//\\/g;
	if ( ! -d $path ) {
		$ret = `MKDIR $path`;
        	if ($?) {
               		print STDERR "$0: Error creating directory $path:\n $!\n";
                	exit (-1);
		}
        }
}
else {
	if ( ! -d $path ) {
		$ret = `mkdir -p $path`;
        	if ($?) {
               		print STDERR "$0: Error creating directory $path:\n $!\n";
                	exit (-1);
		}
		$ret = `chmod g+s $path`;
        	if ($?) {
               		print STDERR "$0 Error setting permissions for $path:\n $!\n";
                	exit (-1);
		}
        }
}

exit(0);

__END__

:endofperl
ccperl -e "$s = shift; $c = $s =~ /.bat$/ ? $s : $s.'.bat'; $p = (-x $c) ? '' :' S '; system('ccperl '.$p.$c.' '.join(' ',@ARGV)); exit $?;" %0 %1 %2 %3 %4 %5 %6 %7 %8 %9

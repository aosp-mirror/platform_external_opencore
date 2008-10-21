@rem = 'Perl, ccperl read this as an array assignment & skip the goto
@echo off
goto endofperl
@rem ';
#!/usr/atria/bin/Perl

######################################################################

# Note that unlink will not remove directories

@cannot = grep {not unlink} @ARGV;

# Make this like Unix rm -f and don't print an error or return a non-zero 
# code if the file doesn't exist
#
# die "$0: Could not remove @cannot\n" if @cannot;

exit (0) if @cannot;

exit(0);

__END__

:endofperl
ccperl -e "$s = shift; $c = $s =~ /.bat$/ ? $s : $s.'.bat'; $p = (-x $c) ? '' :' S '; system('ccperl '.$p.$c.' '.join(' ',@ARGV)); exit $?;" %0 %1 %2 %3 %4 %5 %6 %7 %8 %9

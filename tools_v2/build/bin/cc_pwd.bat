@rem = 'Perl, ccperl read this as an array assignment & skip the goto
@echo off
goto endofperl
@rem ';
#!/usr/atria/bin/Perl

######################################################################

# Instead of 'cleartool pwd' we could also use the Perl module Cwd to
# maintain portability
use Cwd;

$dir = cwd();

# Convert any DOS backslashes to Unix forward slashes
$dir =~ s/\\/\//g;
print "$dir\n";

exit (0)

__END__

:endofperl
ccperl -e "$s = shift; $c = $s =~ /.bat$/ ? $s : $s.'.bat'; $p = (-x $c) ? '' :' S '; system('ccperl '.$p.$c.' '.join(' ',@ARGV)); exit $?;" %0 %1 %2 %3 %4 %5 %6 %7 %8 %9


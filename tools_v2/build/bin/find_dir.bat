@rem = 'Perl, ccperl read this as an array assignment & skip the goto
@echo off
goto endofperl
@rem ';
#!/usr/atria/bin/Perl

######################################################################

{
    local(@whoami) = split('/', $0);
    $whoami = pop(@whoami);
};

if (scalar(@ARGV) != 3)
{
    print STDERR "ARGV = @ARGV scalar = " . scalar(@ARGV) . "\n";
    print STDERR "Usage: $whoami flag dir SRC_TOP\n";
    print STDERR "  Examples: $whoami -I include \${STAGING}\n";
    print STDERR "            $whoami -L libsun \${STAGING}\n";
    exit 1;
}

($flag, $dir, $osrc_top) = @ARGV;

$VOB_BASE_DIR = "$ENV{'VOB_BASE_DIR'}";

chop($pwd = `$VOB_BASE_DIR/tools_v2/build/bin/cc_pwd`);
chdir($osrc_top) or die "Can not change directory to $osrc_top: $!\n";
chop($src_top = `$VOB_BASE_DIR/tools_v2/build/bin/cc_pwd`);

#print "pwd = $pwd\n";

@pwd = split('/', $pwd);
$prefix = "";
@incdirs = ();

#print "pwd array = @pwd\n";

while (@pwd)
{
#	print join('/', @pwd) . "\n";
    chdir(join('/', @pwd)) or die "Can not change directory $!\n";;
    chop($pwd = `$VOB_BASE_DIR/tools_v2/build/bin/cc_pwd`);
    $inc = $pwd;
    $inc =~ s/$src_top/$osrc_top/;
    $inc = "$inc/$dir";
#	print "pwd = $pwd\n";
    push (@incdirs, $inc) if (-d $dir);
    last if ($pwd eq $src_top);
    pop(@pwd);
    $prefix = "../" . $prefix;

}

if (scalar(@pwd) == 0)
{
#    print STDERR "$whoami: Never found $osrc_top\n";
    exit 1;
}

if ($flag ne "0")
{
  for (@incdirs)
    {
      print $flag, $_, " ";
    }
  print "\n";
}
else
{
  for (@incdirs)
    {
      print $_, " ";
    }
  print "\n";
}

__END__

:endofperl
ccperl -e "$s = shift; $c = $s =~ /.bat$/ ? $s : $s.'.bat'; $p = (-x $c) ? '' :' S '; system('ccperl '.$p.$c.' '.join(' ',@ARGV)); exit $?;" %0 %1 %2 %3 %4 %5 %6 %7 %8 %9

@rem = 'Perl, ccperl read this as an array assignment & skip the goto
@echo off
goto endofperl
@rem ';
#!/usr/atria/bin/Perl

######################################################################

use Cwd;

$usage  = "**** Usage: perl $0 <make command> <target> <directory 1> <directory 2> ...\n\n";
$curdir = &Cwd::cwd();

sub make_targets()
{
    foreach $dir (@target_list)
    {
        if (-f "$dir/makefile" || -f "$dir/Makefile")
        {
            if (chdir $dir)
            {
                system("$make_cmd $make_target");
                $exit_status=$?;
                if($exit_status != 0)
                {
                    return $exit_status;
                }
            }
            else
            {
                return $?;
            }
            chdir $curdir;
        }
    }
    return $exit_status;
}

if($#ARGV < 1)
{
    print $usage;
}
else
{
    $make_cmd    = $ARGV[0];
    $make_target = $ARGV[1];
    @target_list = @ARGV[2..$#ARGV];

    # IMPORTANT For the make to exit immediately upon encountering an error, this function
    # should be called at the very end. The exit status of this function then becomes the
    # exit status of the script. This allows makefiles calling this script to exit upon
    # finding a non-zero exit status.
    make_targets();
}

__END__

:endofperl
ccperl -e "$s = shift; $c = $s =~ /.bat$/ ? $s : $s.'.bat'; $p = (-x $c) ? '' :' S '; system('ccperl '.$p.$c.' '.join(' ',@ARGV)); exit $?;" %0 %1 %2 %3 %4 %5 %6 %7 %8 %9
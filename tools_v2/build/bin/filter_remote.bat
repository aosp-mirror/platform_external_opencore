@rem = 'Perl, ccperl read this as an array assignment & skip the goto
@echo off
goto endofperl
@rem ';
#!/usr/atria/bin/Perl

######################################################################

use File::Basename;
my $out="";

if ($#ARGV >= 0) {
    if (!open(FOUT, ">remote.mk")) {
        die "Error opening remote.mk\n";
    }

    while ($_ = shift @ARGV) {
        if (m|/|) {
            $out .= $_ . " ";
            # print "out = $out\n";
            my ($fname, $dirname, $suffix) = fileparse($_, qr/\.[^.]*/);
            my $compiler = "\$(CXX)";
            if ($suffix eq ".c") {
                $compiler = "\$(CC)";
            }
            print FOUT "\$(OBJPAT) : \$(SRCDIR)/$dirname\%$suffix\n";
            print FOUT <<END_OBJRULE;
	\$(create_objdir)
	\$(stage_objdir)
	-\$(RM) \$\@
	$compiler \$(CPPFLAGS) \$(CFLAGS) \$(CO)\$@ \$<
	\$(group_writable)

END_OBJRULE
           print FOUT "\$(DEPPAT) : \$(SRCDIR)/$dirname\%$suffix\n";
           print FOUT <<END_DEPRULE;
	\$(create_objdir)
	\$(stage_objdir)
	\$(GEN_DEPS) \"\$(DEPS_ARG)\" \$(filter \$(\@D)/\%, \$(COMPILED_TARGETS)) \$< \$(\@D) \$(KEEP_OBJ)
	\$(_process_dot_d_)
	\$(_custom_process_dot_d_)

END_DEPRULE
        }
    }
    close(FOUT);

    if (!length($out)) {
        unlink("remote.mk");
    }
    print "$out";
}

__END__

:endofperl
ccperl -e "$s = shift; $c = $s =~ /.bat$/ ? $s : $s.'.bat'; $p = (-x $c) ? '' :' S '; system('ccperl '.$p.$c.' '.join(' ',@ARGV)); exit $?;" %0 %1 %2 %3 %4 %5 %6 %7 %8 %9
 : # Use perl
   eval 'exec perl -S $0 "$@"'
   if 0;

####################################################
#
# This script uses some additional Perl packages that
# are not normally distributed with clearcase perl (called ccperl
# on windows).  The possibilities for handling this are using
# ActiveState's distribution of Perl called ActivePerl or possibly
# checking in the missing Perl packages to clearcase for use with ccperl.
# 
####################################################

 use File::Basename;
 use Getopt::Long;
 use Digest::MD5;

 my $test = 0;
 my $manifest_name;

 if ($#ARGV < 0) {
   print "Must specify the name of the manifest file\n";
   exit(-1);
 }

 $manifest_name = $ARGV[0];

 if (!open(MANIFEST, "$manifest_name")) {
   print "Can't open the file $manifest_name\n";
   exit(-2);
 }

 my $errors = 0;

 while (<MANIFEST>) {
   chomp;

   s/#.*//;
   next if m/^\s*$/;

   my ($fname, $digest) = split '\t';

   #print "filename = $fname, digest = $digest\n";

   my ($status, $computed_digest) = compute_digest($fname);

   if (!$status) {
     print "Error opening file $fname\n";
     ++$errors;
   }

   if ($digest ne $computed_digest) {
     print "Checksum mismatch for file $fname\n";
     ++$errors;
   }

 }

 close MANIFEST;
 exit($errors != 0);
   
 sub compute_digest {
   my $file = shift;

   if (!open(FIN, "$file")) {
     print "Error opening $file\n";
     return (0, undef);
   }
   binmode(FIN);
   
   my $digest = Digest::MD5->new->addfile(*FIN)->hexdigest;

   close FIN;   
   return (1, $digest);


 }




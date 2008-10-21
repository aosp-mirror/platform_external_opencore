# This is a perl script to perform postbuild steps for this project.

{
   local(@whoami) = split('/', $0);
   $whoami = pop(@whoami);
}

sub usage
{
   print "Usage: $whoami <path to lib file to copy> <path to installed_lib dir>\n";
   exit(-1);
}

# ENTRY POINT
$numRequiredArgs = 2;
usage() if ($#ARGV+1 != $numRequiredArgs); # print usage info if number of args is anything other than $numRequiredArgs

# Grab the arguments and store them in local variables
$path_to_lib = $ARGV[0];
if( defined($ENV{CORELIBS_SDK_LOCAL}) ) {
	$path_to_installed_lib = $ENV{CORELIBS_SDK_LOCAL};
}
else {
	$path_to_installed_lib = $ARGV[1];
}

# Create the intalled_lib directory if it does not exist already.
if (! -d $path_to_installed_lib)
{
   print "Directory $path_to_installed_lib does not exist. Attempting to create it.\n";
   if( system( "md $path_to_installed_lib" ) == 0 ) {
	   print "Created directory $path_to_installed_lib\n";
   }
   else {
	   die "Error - Creating directory $path_to_installed_lib Failed";
   }
}

# Copy the lib file if it exists
die "Error - the file does not exist: $path_to_lib." if (! -e $path_to_lib);
if( system( "copy $path_to_lib $path_to_installed_lib" ) == 0 ) {
	print "Copied file $path_to_lib to $path_to_installed_lib successfully.\n";
} 
else {
	die "Error - copy $path_to_lib $path_to_installed_lib Failed";
}


my $filename = $ARGV[0];
print $filename;
open(my $fh, '<', $filename)
  or die "Could not open file '$filename' $!";

while (<$fh>) {

	### solution 3: without prior lc
	$_ =~ s/(?<=[^\A-Z_])_+([^\A-Z_])|([^\A-Z_]+)|_+/\U$1\L$2/g;
	print $_;  
}
close($fh);

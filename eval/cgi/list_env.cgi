# #!/home/mbatstra/.capt/root/usr/bin/perl5.34.0 -wT
use strict;
use CGI qw(:standard);
use CGI::Carp qw(warningsToBrowser fatalsToBrowser);

print header;
print start_html("Environment");

foreach my $key (sort(keys(%ENV))) {
    print "$key = $ENV{$key}<br>\n";
}

print end_html;

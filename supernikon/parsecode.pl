#!/usr/local/bin/perl


foreach $line (<STDIN>) {
  if ($line =~ /(\d+)\s+(\d+)/) {
    print "{$1,\t$2},\n";
  }
}

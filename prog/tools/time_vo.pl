#!/usr/bin/perl
use strict;
use warnings;

package TimeVo;

use Getopt::Long qw(:config posix_default no_ignore_case gnu_compat);

sub MAX_NAME() { 15 };
sub MAX_LEN() { 8 };

my @HOURS = map {sprintf "%02d", $_} 0..23;
my @MINS = map {sprintf "%02d", $_} 0..59;

sub new {
    my ($class) = @_;
    return bless {
        files => [],
        file_ids => {},
        hours => {},
        mins => {},
    }, $class;
}

sub addfile {
    my ($self, $file) = @_;
    my $files = $self->{files};
    my $file_ids = $self->{file_ids};
    if (length($file) > MAX_NAME) {
        print STDERR "too long file: $file\n";
        return -1;
    }
    unless (exists($file_ids->{$file})) {
        my $count = scalar(@$files);
        push @$files, $file;
        $file_ids->{$file} = $count;
    }
    return $file_ids->{$file};
}

sub addfiles {
    my ($self, @files) = @_;
    return grep { $_ >= 0 } map { $self->addfile($_) } @files;
}

sub addhour {
    my ($self, $hour, @files) = @_;
    my @indices = $self->addfiles(@files);
    if (scalar(@indices) > MAX_LEN) {
        print STDERR "too many files in hour$hour: @files\n";
    }
    $self->{hours}->{$hour} = \@indices;
}

sub addmin {
    my ($self, $min, @files) = @_;
    my @indices = $self->addfiles(@files);
    if (scalar(@indices) > MAX_LEN) {
        print STDERR "too many files in min$min: @files\n";
    }
    $self->{mins}->{$min} = \@indices;
}

sub getfiles {
    my ($self, $indices) = @_;
    return map { $self->{files}->[$_] } @$indices;
}

sub checkfiles {
    my ($self, $dir) = @_;
    my $ret = 1;
    for my $file (@{$self->{files}}) {
        my $path = $dir."/".$file;
        unless (-f $path) {
            print "WARN: file $file does not exist in $dir\n";
            $ret = 0;
        }
    }
    return $ret;
}

sub save_bin {
    my ($self, $file) = @_;
    open my $fh, ">", $file or die $!;
    binmode($fh);
    for my $h (@HOURS) {
        my $indices = $self->{hours}->{$h} or die "hour$h is not defined";
        print $fh pack "C".MAX_LEN, @$indices, (255)x MAX_LEN;
    }
    for my $m (@MINS) {
        my $indices = $self->{mins}->{$m} or die "min$m is not defined";
        print $fh pack "C".MAX_LEN, @$indices, (255)x MAX_LEN;
    }
    for my $file (@{$self->{files}}) {
        print $fh pack "Z".(MAX_NAME+1), $file;
    }
    close($fh);
}

sub parse {
    my ($file) = @_;
    my $obj = TimeVo->new;
    my ($n, $fs);
    open my $fh, "<", $file or die $!;
    while (<$fh>) {
        chomp;
        if (($n, $fs) = /hour([01][0-9]|2[0-3]):\s*(.*)\s*/) {
            $obj->addhour($n, split /\s+/, $fs);
            next;
        }
        if (($n, $fs) = /min([0-5][0-9]):\s*(.*)\s*/) {
            $obj->addmin($n, split /\s+/, $fs);
            next;
        }
    }
    close($fh);
    return $obj;
}

sub getopt {
    my ($opts) = @_;

    GetOptions($opts, qw(
        in|i=s
        out|o=s
        dir=s
    ));

    if (exists $opts->{in}) {
        unless (-f $opts->{in}) {
            die "$opts->{in} is not regular file";
        }
        unless (-r $opts->{in}) {
            die "$opts->{in} is not readable";
        }
    }
    if (exists $opts->{out}) {
        if (-e $opts->{out}) {
            unless (-f $opts->{out}) {
                die "$opts->{out} is not regular file";
            }
            unless (-w $opts->{out}) {
                die "$opts->{out} is not writable";
            }
        } else {
            unless (-w dirname($opts->{out})) {
                die "$opts->{out} is not in writable directory";
            }
        }
    }
    if (exists $opts->{dir}) {
        unless (-d $opts->{dir}) {
            die "$opts->{dir} is not directory";
        }
    }
}

sub checkopt {
    my ($opts, @mandate_options) = @_;
    for my $opt (@mandate_options) {
        unless (exists($opts->{$opt})) {
            print STDERR "Missing option '$opt'\n";
            exit 1;
        }
    }
}

package PlayVo;

sub playall {
    my ($obj, $dir) = @_;
    for my $h (@HOURS) {
        my $indices = $obj->{hours}->{$h} or die "hour$h is not defined";
        my @files = map { "$dir/$_" } $obj->getfiles($indices);
        print join(' ', 'play', '-q', @files, '-t', 'alsa')."\n";
    }
    for my $m (@MINS) {
        my $indices = $obj->{mins}->{$m} or die "min$m is not defined";
        my @files = map { "$dir/$_" } $obj->getfiles($indices);
        print join(' ', 'play', '-q', @files, '-t', 'alsa')."\n";
    }
}

sub play {
    my ($obj, $dir, @time) = @_;
    my ($sec, $min, $hour) = localtime();
    $hour = $time[0] if (defined($time[0]));
    $min = $time[1] if (defined($time[1]));
    my $h = $HOURS[int($hour)] or die "Invalid hour $hour";
    my $m = $MINS[int($min)] or die "Invalid minute $min";
    my $h_indices = $obj->{hours}->{$h} or die "hour$h is not defined";
    my $m_indices = $obj->{mins}->{$m} or die "min$m is not defined";
    my @files = map { "$dir/$_" } $obj->getfiles($h_indices), $obj->getfiles($m_indices);
    print join(' ', 'play', '-q', @files, '-t', 'alsa')."\n";
}

package main;

if ($0 eq __FILE__) {
    my %opts;

    my ($command) = shift @ARGV;

    eval { TimeVo::getopt(\%opts) };
    if ($@) {
        print STDERR "$@\n";
        exit 1;
    }

    TimeVo::checkopt(\%opts, qw(in));
    my $obj = TimeVo::parse($opts{in});

    unless (defined($command)) {
        exit 1;
    }
    if ($command eq 'convert') {
        TimeVo::checkopt(\%opts, qw(out));
        $obj->save_bin($opts{out});
        if (exists($opts{dir})) {
            unless ($obj->checkfiles($opts{dir})) {
                exit 1;
            }
        }
        exit 0;
    }
    if ($command eq 'playall') {
        TimeVo::checkopt(\%opts, qw(dir));
        PlayVo::playall($obj, $opts{dir});
        exit 0;
    }
    if ($command eq 'play') {
        PlayVo::play($obj, $opts{dir}, @ARGV);
        exit 0;
    }
}

1;

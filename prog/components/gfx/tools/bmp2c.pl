#!/usr/bin/perl
use strict;
use warnings;

package BitmapHeader;

sub new {
    my ($class) = @_;
    return bless {
        offset => 0,
        width => 0,
        height => 0,
        planes => 0,
        bit_count => 0,
        compression => 0,
        size_image => 0,
        xppm => 0,
        yppm => 0,
        clr_used => 0,
        clr_important => 0,
        palette => [],
    }, $class;
}

sub parse {
    my ($self, $fh) = @_;
    my ($magic, $filesize, $offset, $hdr_data, $hdr_size);
    seek $fh, 0x00, 0;
    read($fh, $magic, 6) == 6 or die $!;
    ($magic, $filesize) = unpack "a2V", $magic;
    unless ($magic eq "BM") {
        die "file is not bitmap";
    }
    seek $fh, 10, 0;
    read($fh, $offset, 4) == 4 or die $!;
    $offset = unpack "V", $offset;
    read($fh, $hdr_data, $offset-14) == $offset-14 or die $!;
    (
        $hdr_size,
        $self->{width},
        $self->{height},
        $self->{planes},
        $self->{bit_count},
        $self->{compression},
        $self->{size_image},
        $self->{xppm},
        $self->{yppm},
        $self->{clr_used},
        $self->{clr_important},
    ) = unpack "VVVvvVVVVVV", $hdr_data;
    if ($self->{bit_count} <= 8) {
        $self->{clr_used} = 1 << $self->{bit_count} unless $self->{clr_used};
    }
    if ($self->{clr_used}) {
        $self->{palette} = [unpack "V"x$self->{clr_used}, substr($hdr_data, $hdr_size)];
    }
    unless ($self->{bit_count} =~ /^(1|2|4|8|16|24|32)$/) {
        die "Unsupported bit count: $self->{bit_count}";
    }
    return $offset;
}

package Bitmap;

sub new {
    my ($class) = @_;
    return bless {
        path => undef,
        header => BitmapHeader->new,
        bitmap => undef,
    }, $class;
}

sub header {
    my ($self) = @_;
    return $self->{header};
}

sub width {
    my ($self) = @_;
    return $self->header->{width};
}

sub height {
    my ($self) = @_;
    return $self->header->{height};
}

sub depth {
    my ($self) = @_;
    return $self->header->{bit_count};
}

sub value {
    my ($self, $x, $y) = @_;
    if ($x < 0 || $x >= $self->width || $y < 0 || $y >= $self->height) {
        return 0;
    }
    my $row = $self->{bitmap}->[$y];
    return ${$row}[$x];
}

sub parse_row_1bit {
    my @row = @_;
    my @values = ();
    for my $v (@row) {
        for my $i (0..7) {
            push @values, ($v>>(7-$i))&1;
        }
    }
    return @values;
}

sub parse_row_2bit {
    my @row = @_;
    my @values = ();
    for my $v (@row) {
        for my $i (0..3) {
            push @values, ($v>>(6-$i*2))&3;
        }
    }
    return @values;
}

sub parse_row_4bit {
    my @row = @_;
    my @values = ();
    for my $v (@row) {
        push @values, ($v>>4)&0x0f;
        push @values, ($v)&0x0f;
    }
    return @values;
}

sub parse_row_8bit {
    return @_;
}

sub parse_row_16bit {
    my @row = @_;
    my @values = ();
    while (@row >= 2) {
        my ($l, $h) = (shift @row, 2);
        push @values, ($h<<8)|$l;
    }
    return @values;
}

sub parse_row_24bit {
    my @row = @_;
    my @values = ();
    while (@row >= 3) {
        my ($l, $h, $x) = (shift @row, 3);
        push @values, ($x<<16)|($h<<8)|$l;
    }
    return @values;
}

sub parse_row_32bit {
    my @row = @_;
    my @values = ();
    while (@row >= 4) {
        my ($l, $h, $x, $xh) = (shift @row, 4);
        push @values, ($xh<<24)|($x<<16)|($h<<8)|$l;
    }
    return @values;
}

sub parse {
    my ($self, $fh) = @_;
    my $header = $self->header;
    my $offset = $header->parse($fh) or die $!;
    seek($fh, $offset, 0);
    my $rowsize = (int(($self->width*$self->depth+7)/8)+3)&~3;
    my $bit_count = $self->depth;
    my $row;
    my @bitmap = ();
    for my $y (1..$self->height) {
        read($fh, $row, $rowsize) == $rowsize or die $!;
        my @row = eval "parse_row_${bit_count}bit(unpack \"c$rowsize\", \$row)";
        $#row = $self->width-1;
        $bitmap[$header->{height}-$y] = \@row;
    }
    $self->{bitmap} = \@bitmap;
}

package main;

use File::Basename qw(dirname);
use Getopt::Long qw(:config posix_default no_ignore_case gnu_compat);

sub getopt {
    my ($opts) = @_;

    my @MANDATORY_OPTIONS = qw(in out);
    GetOptions($opts, qw(
        in|i=s
        out|o=s
        depth|d=i
    ));

    for my $opt (@MANDATORY_OPTIONS) {
        unless (exists $opts->{$opt}) {
            print STDERR "Missing option '$opt'\n";
            exit 1;
        }
    }
    if (-e $opts->{in}) {
        unless (-f $opts->{in}) {
            print STDERR "$opts->{in} is not regular file\n";
            exit 1;
        }
        unless (-r $opts->{in}) {
            print STDERR "$opts->{in} is not readable\n";
            exit 1;
        }
    } else {
        print STDERR "$opts->{in} does not exist\n";
        exit 1;
    }
    if (-e $opts->{out}) {
        unless (-f $opts->{out}) {
            print STDERR "$opts->{out} is not regular file\n";
            exit 1;
        }
        unless (-w $opts->{out}) {
            print STDERR "$opts->{out} is not writable\n";
            exit 1;
        }
    } else {
        unless (-w dirname($opts->{out})) {
            print STDERR "$opts->{out} is not in writable directory\n";
            exit 1;
        }
    }
}

sub path_to_identifier {
    my ($path) = @_;
    $path =~ s#.*/##;
    $path =~ s#[^a-zA-Z0-9_]#_#g;
    $path = '_'.$path if $path =~ /^[0-9]/;
    return $path;
}

my %opts;

getopt(\%opts);
$opts{depth} ||= 1;

my @lines = ();

open my $fh, "<", $opts{in} or die $!;
binmode($fh);
my $bitmap = Bitmap->new;
$bitmap->parse($fh);
close $fh;


open $fh, ">", $opts{out};
print $fh "#include <gfx_bitmap.h>\n\n";
print $fh "static const uint8_t data[] = {\n";
for my $y (1..$bitmap->height) {
    print $fh " ";
    my $value = 0;
    my $n = 0;
    for my $x (1..(($bitmap->width+7)&~7)) {
        my $color = $bitmap->value($x-1, $y-1);
        if ($color) {
            $value |= 0x80>>$n;
        }
        $n++;
        if ($n == 8) {
            printf $fh " 0x%02x,", $value;
            $value = 0;
            $n = 0;
        }
    }
    print $fh "\n";
}
print $fh "};\n";
print $fh "const gfx_bitmap_t ".path_to_identifier($opts{in})." = {\n";
print $fh "  .header = {\n";
print $fh "    .width = ", $bitmap->width, ",\n";
print $fh "    .height = ", $bitmap->height, ",\n";
print $fh "    .scansize = ", int(($bitmap->width*$opts{depth}+7)/8), ",\n";
print $fh "    .depth = ", $opts{depth}, ",\n";
print $fh "  },\n";
print $fh " .data = (uint8_t*)data,\n";
print $fh "};\n";

exit 0;

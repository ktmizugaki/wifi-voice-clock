#!/usr/bin/perl
use strict;
use warnings;
use File::Basename;

BEGIN {
    require "".File::Basename::dirname(__FILE__)."/lang.pl";
}

package ConvChar;

use Encode qw(decode);

sub jis2ucs {
    my ($jis) = @_;
    my $ucs;
    if ($jis >= 20 && $jis <= 127) {
        return $jis;
    }
    $ucs = ord(decode("ISO-2022-JP", "\x1b\x24\x42".pack("CC", $jis>>8, $jis&255)."\x1b\x28\x42"));
    return $ucs || $jis;
}

package Glyph;

sub new {
    my ($class, $font) = @_;
    die "Invalid argument" unless $font;
    return bless {
        font => $font,
        index => undef,
        unicode => undef,
        swidth => $font->{'swidth'},
        dwidth => $font->{'dwidth'},
        bbx => $font->{'bbx'},
        bitmap => [],
    }, $class;
}

sub x_advance {
    my ($self) = @_;
    if (defined($self->{'dwidth'})) {
        return $self->{'dwidth'};
    }
    return 0;
}

sub width {
    my ($self) = @_;
    if (defined($self->{'bbx'})) {
        return $self->{'bbx'}->{'w'};
    }
    if (defined($self->{'dwidth'})) {
        return $self->{'dwidth'};
    }
    return 0;
}

sub height {
    my ($self) = @_;
    if (defined($self->{'bbx'})) {
        return $self->{'bbx'}->{'h'};
    }
    if (defined($self->{'font'}->height)) {
        return $self->{'font'}->height;
    }
    return 0;
}

sub x_offset {
    my ($self) = @_;
    if (defined($self->{'bbx'})) {
        return $self->{'bbx'}->{'x'};
    }
    return 0;
}

sub y_offset {
    my ($self) = @_;
    if (defined($self->{'bbx'})) {
        return $self->{'font'}->{'font_ascent'} - $self->{'bbx'}->{'y'} - $self->{'font'}->height;
    }
    return 0;
}

sub bitmap_length {
    my ($self) = @_;
    my $w = $self->width;
    if ($w) {
        return int(($w+7)/8)*2;
    }
    return 0;
}

sub bitmap_size {
    my ($self) = @_;
    my $w = $self->width;
    if ($w) {
        return int(($w+7)/8)*scalar(@{$self->{'bitmap'}});
    }
    return 0;
}

sub glyph_binary {
    my ($self, $bitmap_offset) = @_;
    return pack "vvCCcc",
        $self->{'unicode'}, # uint16_t unicode
        $bitmap_offset,     # uint16_t bitmap_offset
        $self->width,       # uint16_t width
        $self->x_advance,   # x_advance
        $self->x_offset,    # x_offset
        $self->y_offset;    # y_offset
}

sub bitmap_data {
    my ($self) = @_;
    my $data = '';
    for my $row (@{$self->{'bitmap'}}) {
        my @arr = map {hex($_)} ( $row =~ m/../g );
        $data .= pack "C*", @arr;
    }
    return $data;
}

sub is_valid {
    my ($self) = @_;
    return defined($self->{'unicode'}) &&
        defined($self->{'swidth'}) &&
        defined($self->{'dwidth'}) &&
        defined($self->{'bbx'}) &&
        ($#{$self->{'bitmap'}} == $self->{'font'}->height-1);
}

sub bbx_str {
    my ($bbx) = @_;
    return "w:".$bbx->{w}.",h:".$bbx->{h}.",x:".$bbx->{x}.",y:".$bbx->{y};
}

package Font;

sub new {
    my ($class) = @_;
    return bless {
        version => "KT01",
        default_char => undef,
        font_ascent => undef,
        font_descent => undef,
        chars => undef,
        swidth => undef,
        dwidth => undef,
        bbx => undef,
        glyphs => [],
    }, $class;
}

sub merge {
    my ($self, $other) = @_;
    unless ($self->height == $other->height) {
        die "incompatible font to merge (height differ)";
    }
    push @{$self->{'glyphs'}}, grep { !$self->glyph($_->{'unicode'}) } @{$other->{'glyphs'}};
}

sub height {
    my ($self) = @_;
    if (defined($self->{font_ascent}) && defined($self->{font_descent})) {
        return $self->{font_ascent} + $self->{font_descent};
    } else {
        return -1
    }
}

sub default_char {
    my ($self, $fallback) = @_;
    if (defined($self->{'default_char'})) {
        return $self->{'default_char'};
    }
    return $fallback;
}

sub glyph {
    my ($self, $unicode) = @_;
    for my $glyph (@{$self->{'glyphs'}}) {
        if ($glyph->{'unicode'} == $unicode) {
            return $glyph;
        }
    }
}

package BDFParser;

use constant DEBUG => 0;

sub to_unicode {
    my ($code, $opts) = @_;
    my $charset = $opts->{charset} || 'utf16';
    if ($charset eq 'utf16') {
        return $code;
    } elsif ($charset eq 'jis') {
        return ConvChar::jis2ucs($code);
    } else {
        print STDERR "ERR: unknwon charset: $charset\n";
        return $code;
    }
}

sub parse {
    my ($fh, $opts) = @_;
    my $font = Font->new();
    my $glyph;
    my $state = 0;
    # 0=header, 1=char, 2=bitmap;
    while (<$fh>) {
        s/\s+$//;
        if ($state == 0) {
            if (/^STARTCHAR /) {
                $state = 1;
            } elsif (/^DEFAULT_CHAR +(\d+)/) {
                $font->{'default_char'} = to_unicode(int($1), $opts);
            } elsif (/^FONT_ASCENT +(-?\d+)/) {
                $font->{'font_ascent'} = int($1);
            } elsif (/^FONT_DESCENT +(-?\d+)/) {
                $font->{'font_descent'} = int($1);
            } elsif (/^CHARS +(\d+)/) {
                $font->{'chars'} = int($1);
            } elsif (/^SWIDTH +(\d+) +(\d+)/) {
                $font->{'swidth'} = int($1);
            } elsif (/^DWIDTH +(\d+) +(\d+)/) {
                $font->{'dwidth'} = int($1);
            } elsif (/^FONTBOUNDINGBOX +(-?\d+) +(-?\d+) +(-?\d+) +(-?\d+)/) {
                $font->{'bbx'} = {'w' => int($1), 'h' => int($2), 'x' => int($3), 'y' => int($4)};
            }
        }
        if ($state == 2) {
            if (/^STARTCHAR /) {
                $state = 1;
            } elsif (/^ENDCHAR/) {
                unless ($glyph->is_valid) {
                    print STDERR "Invalid glyph: ", $glyph->{'index'},
                        (defined($glyph->{'unicode'})? (', unicode=',$glyph->{'unicode'}): ''), "\n";
                    die;
                } else {
                    if (DEBUG) {
                        print STDERR "glyph ", $glyph->{'index'}, ": unicode=", $glyph->{'unicode'},
                            ", width=", $glyph->{'dwidth'}, ", bbx=", Glyph::bbx_str($glyph->{'bbx'}),
                            ", bitmap=", join(",", @{$glyph->{'bitmap'}}), "\n";
                    }
                    push @{$font->{'glyphs'}}, $glyph;
                }
                $glyph = undef;
                $state = 1;
                next;
            } else {
                unless (length($_) == $glyph->bitmap_length) {
                    print STDERR "Invalid bitmap row length: $_: ", length($_), "/", $glyph->bitmap_length, "\n";
                }
                push @{$glyph->{'bitmap'}}, $_;
            }
        }
        if ($state == 1) {
            if (/^STARTCHAR +([\da-fA-F]+)/) {
                if ($glyph) {
                    print STDERR "Invalid glyph: ", $glyph->{'index'}, ($glyph->{'unicode'}? (', unicode=',$glyph->{'unicode'}): ''), "\n";
                }
                $glyph = Glyph->new($font);
                $glyph->{'index'} = hex($1);
            } elsif (/^ENCODING +(\d+)/) {
                $glyph->{'unicode'} = to_unicode(int($1), $opts);
            } elsif (/^SWIDTH +(\d+) +(\d+)/) {
                $glyph->{'swidth'} = int($1);
            } elsif (/^DWIDTH +(\d+) +(\d+)/) {
                $glyph->{'dwidth'} = int($1);
            } elsif (/^BBX +(-?\d+) +(-?\d+) +(-?\d+) +(-?\d+)/) {
                $glyph->{'bbx'} = {'w' => int($1), 'h' => int($2), 'x' => int($3), 'y' => int($4)};
            } elsif (/^BITMAP/) {
                $state = 2;
            }
        }
        if (/^ENDFONT/) {
            if ($state == 1 && $glyph) {
                print STDERR "Invalid glyph: ", $glyph->{'index'}, ($glyph->{'unicode'}? (', unicode=',$glyph->{'unicode'}): ''), "\n";
            } elsif ($state == 2) {
                print STDERR "Invalid glyph: ", $glyph->{'index'}, ($glyph->{'unicode'}? (', unicode=',$glyph->{'unicode'}): ''), "\n";
            }
        }
    }
    return $font;
}

package main;

use File::Basename qw(dirname);
use Getopt::Long qw(:config posix_default no_ignore_case gnu_compat);

sub getopt {
    my ($opts) = @_;

    my @MANDATORY_OPTIONS = qw(in out);
    GetOptions($opts, qw(
        in|i=s@
        out|o=s
        charset|c=s
        lang|=s
    ));

    for my $opt (@MANDATORY_OPTIONS) {
        unless (exists $opts->{$opt}) {
            print STDERR "Missing option '$opt'\n";
            exit 1;
        }
    }
    for my $in (@{$opts->{in}}) {
        unless (-f $in) {
            print STDERR "$in is not regular file\n";
            exit 1;
        }
        unless (-r $in) {
            print STDERR "$in is not readable\n";
            exit 1;
        }
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

my %opts;
my ($font, $lang);

getopt(\%opts);

if (defined($opts{in})) {
    for my $in (@{$opts{in}}) {
        open my $fh, "<", $in or die $!;
        my $tmp = BDFParser::parse($fh, \%opts);
        close($fh);
        if ($font) {
            $font->merge($tmp);
        } else {
            $font = $tmp;
        }
    }
}

if (defined($opts{lang})) {
    open my $fh, "<", $opts{lang} or die $!;
    $lang = Lang->new->load($fh);
    close($fh);
}

my $default_char = $font->default_char(32);
my @glyphs = sort { $a->{'unicode'} - $b->{'unicode'} } @{$font->{'glyphs'}};
if ($lang) {
    my %chars = %{$lang->chars};
    $chars{$default_char} = undef;
    @glyphs = grep { exists $chars{$_->{'unicode'}} } @glyphs;
}
my $glyph_count = scalar(@glyphs);
if ($glyph_count == 0) {
    printf STDERR "WARN: No glyph to output\n";
}
open my $fh, ">", $opts{out} or die $!;
binmode($fh);
print $fh $font->{'version'};
print $fh pack "S", $glyph_count; # glyph_count
print $fh pack "S", $glyph_count == 0? 0: $glyphs[0]->{'unicode'}; # first
print $fh pack "S", $glyph_count == 0? 0: $glyphs[$glyph_count-1]->{'unicode'}; # last
print $fh pack "S", $default_char; # default_char
print $fh pack "C", $font->height; # height
print $fh pack "C3", 0, 0, 0; # padding
my $offset = 0;
for my $glyph (@glyphs) {
    print $fh $glyph->glyph_binary($offset);
    $offset += $glyph->bitmap_size;
}
for my $glyph (@glyphs) {
    print $fh $glyph->bitmap_data; # bitmap
}
close($fh);

print "Wrote $glyph_count glyphs\n";

exit 0;

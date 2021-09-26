#!/usr/bin/perl
use strict;
use warnings;

package Lang;

use Encode qw(encode decode);
use constant DEBUG => 0;

sub parse_cstr {
    my ($cstr) = @_;
    # TODO: parse escape characters;
    return $cstr;
}

sub new {
    my ($class) = @_;
    return bless {
        keys => [],
        strs => {},
        ids => {},
        chars => undef,
    }, $class;
}

sub keys {
    return $_[0]->{'keys'};
}

sub strs {
    return $_[0]->{'strs'};
}

sub ids {
    return $_[0]->{'ids'};
}

sub load {
    my ($self, $fh) = @_;
    while (<$fh>) {
        if (/^#/) {
            next;
        }
        if (my ($key, $str) = /^\s*([A-Z_]+): (.*)/) {
            $self->add($key, $str);
            next;
        }
    }
    return $self;
}

sub save_header {
    my ($self, $fh) = @_;
    for my $key (@{$self->keys}) {
        my $str = $self->strs->{$key};
        my $lang_id = $self->ids->{$key};
        print $fh "#define LANG_$key \"$str\"\n";
        print $fh "#define LANGID_$key $lang_id\n";
    }
    return $self;
}

sub add {
    my ($self, $key, $str) = @_;
    if (exists $self->strs->{$key}) {
        print STDERR "WARN: duplicate key: $key";
    }
    my $lang_id = scalar(@{$self->keys});
    push @{$self->keys}, $key;
    $self->strs->{$key} = $str;
    $self->ids->{$key} = $lang_id;
    $self->{'chars'} = undef;
}

sub chars {
    my ($self) = @_;
    unless (defined($self->{'chars'})) {
        my $chars = $self->{'chars'} = {};
        for my $str (values %{$self->strs}) {
            $str = parse_cstr($str);
            for my $char (split //, decode('utf-8', $str)) {
                $chars->{ord($char)} = undef;
            }
        }
    }
    return $self->{'chars'};
}

package main;

if ($0 eq __FILE__) {
    print $0, "\n";
    my ($in, $out) = @ARGV;
    my $fh;
    my $lang = Lang->new;

    open $fh, "<", $in or die $!;
    $lang->load($fh);
    close $fh;

    open $fh, ">", $out or die $!;
    print $fh <<EOS;
/* This file is auto generated from $in */
EOS
    $lang->save_header($fh);
    close $fh;
}

1;

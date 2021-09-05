#!/usr/bin/perl -T
use strict;
use warnings;
use File::Basename;
use HTTP::Daemon;
use HTTP::Status;

$ENV{PATH} = '/bin:/usr/bin';

my $cmn_path = File::Basename::dirname(__FILE__);

our @HANDLERS = ();

sub cmn_test_handler {
    my ($c, $req) = @_;
    if ($req->method eq 'GET' && $req->uri->path eq '/') {
        return $c->send_file_response($cmn_path.'/cmn-test.html');
    }
    return undef;
}

sub cmn_handler {
    my ($c, $req) = @_;
    if ($req->method eq 'GET' && $req->uri->path eq '/cmn.js') {
        return $c->send_file_response($cmn_path.'/cmn.js');
    } elsif ($req->method eq 'GET' && $req->uri->path eq '/cmn.css') {
        return $c->send_file_response($cmn_path.'/cmn.css');
    }
    return undef;
}

sub add_handlers {
    push @HANDLERS, @_;
}

sub get_handlers {
    return \@HANDLERS;
}

sub parse_query {
    my ($query) = @_;
    # TODO: url decode
    return map {split /=/, $_, 2} split /&/, $query, -1;
}

sub is_true_like {
    defined($_[0]) && $_[0] =~ /^(1|y|t|yes|true|on)$/;
}

sub is_false_like {
    defined($_[0]) && $_[0] =~ /^(0|n|f|no|false|off)$/;
}

sub to_json_bool {
    my ($value) = @_;
    if (is_true_like($value)) {
        return 'true';
    } else {
        return 'false';
    }
}

sub json_response {
    my ($json) = @_;
    return HTTP::Response->new("200", "OK", ["Content-Type" => "application/json"], $json);
}

sub start_httpd {
    my ($path) = @_;
    my $port = shift;
    if ($port && $port =~ /^([0-9]+)$/) {
        $port = int($1);
    } else {
        $port = undef;
    }

    my $d = HTTP::Daemon->new(
            LocalAddr => '0.0.0.0',
            LocalPort => $port || 8080
        ) or die $!;
    print "Listening on ", $d->url, $path||"", "\n";
    return $d;
}

sub run_httpd {
    my ($d) = @_;
    while ( my ( $c, $peer_addr ) = $d->accept ) {
        if ( my $req = $c->get_request ) {
            my $query = $req->uri->query || "";
            my $res;
            for my $handler (@HANDLERS) {
                if (($res = $handler->($c, $req))) {
                    last;
                }
            }
            unless ($res) {
                $res = $c->send_error(HTTP::Status::HTTP_NOT_FOUND);
            }
            print $req->method, " ", $req->uri->path, " => ", (ref $res ? $res->code : $res), "\n";
        }
        $c->close;
        undef($c);
    }
    $d->close;
}

if ($0 eq __FILE__) {
    add_handlers(\&cmn_test_handler, \&cmn_handler);
    run_httpd(start_httpd());
}

1;
__END__

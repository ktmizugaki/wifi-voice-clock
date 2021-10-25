#!/usr/bin/perl -T
use strict;
use warnings;
use File::Basename;
use Cwd "cwd";
use JSON;

$ENV{PATH} = '/bin:/usr/bin';

my $firmware_base = File::Basename::dirname(__FILE__);

our $html_cmn;
unless (defined($html_cmn)) {
    chomp(my $cmn_path = `find "$firmware_base/../.." -type d -name http_html_cmn | head -1`);
    if ($cmn_path =~ m#^([-\@\w./]+$)#) {
        $cmn_path = $1."/html";
    } else {
        die "http_html_cmn is not found";
    }
    require "$cmn_path/test-html.pl";
}

my $app_version = "v3.0.0-perl";
my $app_datetime = "Jul 07 2021 07:45:21";
my $idf_version = "v4.0";

sub firmware_handler {
    my ($c, $req) = @_;
    if ($req->method eq 'GET' && $req->uri->path eq '/firmware') {
        return $c->send_file_response($firmware_base.'/http_firmware.html');
    } elsif ($req->method eq 'GET' && $req->uri->path eq '/firmware/version') {
        my $json = encode_json({
            'app_version' => $app_version,
            'datetime' => $app_datetime,
            'idf_version' => $idf_version,
        });
        my $res = json_response($json);
        $c->send_response($res);
        return $res;
    } elsif ($req->method eq 'POST' && $req->uri->path eq '/firmware/update') {
        my $json;
        $json = '{"status":0,"message":"Not implemented"}';
        my $res = json_response($json);
        $c->send_response($res);
        return $res;
    } elsif ($req->method eq 'POST' && $req->uri->path eq '/firmware/spiffs') {
        my $json;
        $json = '{"status":0,"message":"Not implemented"}';
        my $res = json_response($json);
        $c->send_response($res);
        return $res;
    }
    return undef;
}

if ($0 eq __FILE__) {
    add_handlers(\&cmn_handler, \&firmware_handler);
    run_httpd(start_httpd("firmware"));
}

1;
__END__

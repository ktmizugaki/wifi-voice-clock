#!/usr/bin/perl -T
use strict;
use warnings;
use File::Basename;
use Cwd "cwd";

$ENV{PATH} = '/bin:/usr/bin';

my $clock_conf_base = File::Basename::dirname(__FILE__);

our $html_cmn;
unless (defined($html_cmn)) {
    chomp(my $cmn_path = `find "$clock_conf_base/../.." -type d -name http_html_cmn | head -1`);
    if ($cmn_path =~ m#^([-\@\w./]+$)#) {
        $cmn_path = $1."/html";
    } else {
        die "http_html_cmn is not found";
    }
    require "$cmn_path/test-html.pl";
}

my $clock_conf = {TZ => 'JST-9', sync_weeks=>0x02, sync_time=>0};

sub clock_conf_handler {
    my ($c, $req) = @_;
    if ($req->method eq 'GET' && $req->uri->path eq '/clock_conf') {
        return $c->send_file_response($clock_conf_base.'/http_clock_conf.html');
    } elsif ($req->method eq 'GET' && $req->uri->path eq '/clock_conf/conf') {
        my $json = '{"status":1,"TZ":"'.$clock_conf->{'TZ'}.'",'.
            '"sync_weeks":'.$clock_conf->{'sync_weeks'}.','.
            '"sync_time":'.$clock_conf->{'sync_time'}.'}';
        my $res = json_response($json);
        $c->send_response($res);
        return $res;
    } elsif ($req->method eq 'POST' && $req->uri->path eq '/clock_conf/conf') {
        my %params = parse_query($req->decoded_content);
        my $json;
        if (!defined($params{'TZ'}) || !defined($params{'sync_weeks'}) || !defined($params{'sync_time'})) {
            $json = '{"status":0,"message":"Missing params"}';
        } else {
            $clock_conf->{'TZ'} = $params{'TZ'};
            $clock_conf->{'sync_weeks'} = $params{'sync_weeks'};
            $clock_conf->{'sync_time'} = $params{'sync_time'};
            $json = '{"status":1,"message":"Updated clock conf"}';
        }
        my $res = json_response($json);
        $c->send_response($res);
        return $res;
    } elsif ($req->method eq 'GET' && $req->uri->path eq '/clock_conf/time') {
        my $json = '{"status":1,"time":'.time().'}';;
        my $res = json_response($json);
        $c->send_response($res);
        return $res;
    } elsif ($req->method eq 'POST' && $req->uri->path eq '/clock_conf/time') {
        my $json = '{"status":1,"message":"Updated time"}';
        my $res = json_response($json);
        $c->send_response($res);
        return $res;
    }
    return undef;
}

if ($0 eq __FILE__) {
    add_handlers(\&cmn_handler, \&clock_conf_handler);
    run_httpd(start_httpd("clock_conf"));
}

1;
__END__

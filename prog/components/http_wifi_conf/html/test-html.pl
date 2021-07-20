#!/usr/bin/perl -T
use strict;
use warnings;
use File::Basename;
use Cwd "cwd";

$ENV{PATH} = '/bin:/usr/bin';

my $wifi_conf_base = File::Basename::dirname(__FILE__);

chomp(my $cmn_path = `find "$wifi_conf_base/../.." -type d -name http_html_cmn | head -1`);
if ($cmn_path =~ m#^([-\@\w./]+$)#) {
    $cmn_path = $1."/html";
} else {
    die "http_html_cmn is not found";
}

require "$cmn_path/test-html.pl";

my $scan_req_count = 0;
my $max_ap = 3;
my @aps = ();
my $conn = undef;

sub find_ap {
    my ($ssid) = @_;
    my $i;
    return -1 unless $ssid;
    for ($i = $#aps; $i >= 0; $i--) {
        return $i if ($aps[$i] =~ /"ssid":"$ssid"/);
    }
    return $i;
}

sub wifi_conf_handler {
    my ($c, $req) = @_;
    if ($req->method eq 'GET' && $req->uri->path eq '/wifi_conf') {
        return $c->send_file_response($wifi_conf_base.'/http_wifi_conf.html');
    } elsif ($req->uri->path eq '/wifi_conf/scan') {
        my $json;
        if ($req->method eq 'GET' && ($scan_req_count == 0 || $scan_req_count == 10)) {
            $json = '{"status":0}';
        } elsif ($scan_req_count++ < 5) {
            $json = '{"status":1}';
        } elsif ($scan_req_count < 10) {
            $json = '{"status":2,"records":[{"ssid":"SSID1","rssi":-65},{"ssid":"SSID2","rssi":-80,"authmode":"WPA2-AES"}]}';
        } elsif ($scan_req_count < 15) {
            $json = '{"status":1}';
        } elsif ($scan_req_count < 20) {
            $json = '{"status":2,"records":[]}';
        } else {
            $scan_req_count = 0;
            $json = '{"status":0}';
        }
        my $res = json_response($json);
        $c->send_response($res);
        return $res;
    } elsif ($req->method eq 'GET' && $req->uri->path eq '/wifi_conf/aps') {
        my $json;
        $json = '{"status":1,"count":'.scalar(@aps).',"aps":['.join(',', @aps).']}';
        my $res = json_response($json);
        $c->send_response($res);
        return $res;
    } elsif ($req->method eq 'POST' && $req->uri->path eq '/wifi_conf/aps') {
        my %params = parse_query($req->decoded_content);
        my $json;
        my $i = find_ap($params{ssid});
        if ($i == -1 && scalar(@aps) >= $max_ap) {
            $json = '{"status":0,"message":"Too many aps"}';
        } elsif ($params{ssid} && ($i >= 0 || $params{password}) && $params{ntp}) {
            my $ap = '"ssid":"'.$params{ssid}.'","use_static_ip":'.to_json_bool($params{use_static_ip}).',"ntp":"'.$params{ntp}.'"';
            if ($params{use_static_ip} eq 'true') {
                $ap .= ',"ip":"'.$params{ip}.'","gateway":"'.$params{gateway}.'","netmask":"'.$params{netmask}.'"';
            }
            $ap = '{'.$ap.'}';
            if ($i >= 0) {
                $aps[$i] = $ap;
                $json = '{"status":1,"message":"Updated ap '.$params{ssid}.'"}';
            } else {
                push @aps, $ap;
                $json = '{"status":1,"message":"Added ap '.$params{ssid}.'"}';
            }
        } else {
            $json = '{"status":0,"message":"Missing params"}';
        }
        my $res = json_response($json);
        $c->send_response($res);
        return $res;
    } elsif ($req->method eq 'DELETE' && $req->uri->path eq '/wifi_conf/aps') {
        my %params = parse_query($req->decoded_content);
        my $ssid = $params{ssid};
        my $json;
        my $i = find_ap($ssid);
        if ($i > -1) {
            splice @aps, $i, 1;
            $json = '{"status":1,"message":"Removed ap '.$params{ssid}.'"}';
        } else {
            $json = '{"status":0,"message":"Unknown ap '.$params{ssid}.'"}';
        }
        my $res = json_response($json);
        $c->send_response($res);
        return $res;
    } elsif ($req->method eq 'GET' && $req->uri->path eq '/wifi_conf/conn') {
        my $json;
        if ($conn) {
            $json = '{"status":2,"ssid":"'.$conn.'","rssi":-70,"ip":"192.168.1.10","message":"Connected to ap '.$conn.'"}';
        } else {
            $json = '{"status":0}';
        }
        my $res = json_response($json);
        $c->send_response($res);
        return $res;
    } elsif ($req->method eq 'POST' && $req->uri->path eq '/wifi_conf/conn') {
        my %params = parse_query($req->decoded_content);
        my $json;
        if ($params{ssid}) {
            $conn = $params{ssid};
            $json = '{"status":1,"message":"Connecting"}';
        } else {
            $json = '{"status":0,"message":"Missing params"}';
        }
        my $res = json_response($json);
        $c->send_response($res);
        return $res;
    } elsif ($req->method eq 'DELETE' && $req->uri->path eq '/wifi_conf/conn') {
        my $json;
        if ($conn) {
            $json = '{"status":1,"message":"Disconnected from '.$conn.'"}';
            $conn = undef;
        } else {
            $json = '{"status":0,"message":"Not connected"}';
        }
        my $res = json_response($json);
        $c->send_response($res);
        return $res;
    }
    return undef;
}

if ($0 eq __FILE__) {
    add_handlers(\&cmn_handler, \&wifi_conf_handler);
    run_httpd(start_httpd("wifi_conf"));
}

1;
__END__

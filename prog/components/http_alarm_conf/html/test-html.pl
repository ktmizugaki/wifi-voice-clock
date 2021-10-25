#!/usr/bin/perl -T
use strict;
use warnings;
use File::Basename;
use Cwd "cwd";
use JSON;

$ENV{PATH} = '/bin:/usr/bin';

my $alarm_conf_base = File::Basename::dirname(__FILE__);

our $html_cmn;
unless (defined($html_cmn)) {
    chomp(my $cmn_path = `find "$alarm_conf_base/../.." -type d -name http_html_cmn | head -1`);
    if ($cmn_path =~ m#^([-\@\w./]+$)#) {
        $cmn_path = $1."/html";
    } else {
        die "http_html_cmn is not found";
    }
    require "$cmn_path/test-html.pl";
}

my $num_alarm = 5;
my $num_alarm_sound = 3;
my @alarms = map { {'enabled'=>0, 'name'=>'', 'weeks'=>0x7f, 'alarm_id'=>0, 'seconds'=>0} } (1..$num_alarm);

sub alarm_validate_params {
    my (%params) = @_;
    my ($index, $enabled, $name, $weeks, $alarm_id, $seconds) = ($params{'index'}, $params{'enabled'}, $params{'name'}, $params{'weeks'}, $params{'alarm_id'}, $params{'seconds'});
    unless (defined($index) && defined($enabled) && defined($name) && defined($weeks) && defined($alarm_id) && defined($seconds)) {
        return "Missing params";
    }
    unless ($index =~ /^[0-9]+$/ && int($index) >= 0 && int($index) < $num_alarm) {
        return "Invalid index";
    }
    unless (is_true_like($enabled) || is_false_like($enabled)) {
        return "Invalid enabled";
    }
    unless ($name =~ /^.{0,10}$/) {
        return "Invalid name";
    }
    unless ($weeks =~ /^[0-9]+$/ && int($weeks) >= 0 && int($weeks) <= 0x7f) {
        return "Invalid weeks";
    }
    unless ($alarm_id =~ /^[0-9]+$/ && int($alarm_id) >= 0 && int($alarm_id) < $num_alarm_sound) {
        return "Invalid alarm_id";
    }
    unless ($seconds =~ /^-?[0-9]+$/ && int($seconds) >= 0 && int($seconds) < 86400) {
        return "Invalid seconds";
    }
    return undef;
}

sub alarm_conf_handler {
    my ($c, $req) = @_;
    if ($req->method eq 'GET' && $req->uri->path eq '/alarm_conf') {
        return $c->send_file_response($alarm_conf_base.'/http_alarm_conf.html');
    } elsif ($req->method eq 'GET' && $req->uri->path eq '/alarm_conf/alarms') {
        my $json = '{"status":1,"num_alarm_sound":'.$num_alarm_sound.',"alarms":'.encode_json(\@alarms).'}';
        my $res = json_response($json);
        $c->send_response($res);
        return $res;
    } elsif ($req->method eq 'POST' && $req->uri->path eq '/alarm_conf/alarms') {
        my %params = parse_query($req->decoded_content);
        my $json;
        my $message = alarm_validate_params(%params);
        if ($message) {
            $json = '{"status":0,"message":"'.$message.'"}';
        } else {
            $alarms[$params{'index'}] = {
                'enabled'=>is_true_like($params{'enabled'}),
                'name'=>$params{'name'},
                'weeks'=>int($params{'weeks'}),
                'alarm_id'=>int($params{'alarm_id'}),
                'seconds'=>int($params{'seconds'}),
            };
            $json = '{"status":1,"message":"Updated Alarm"}';
        }
        my $res = json_response($json);
        $c->send_response($res);
        return $res;
    }
    return undef;
}

if ($0 eq __FILE__) {
    add_handlers(\&cmn_handler, \&alarm_conf_handler);
    run_httpd(start_httpd("alarm_conf"));
}

1;
__END__

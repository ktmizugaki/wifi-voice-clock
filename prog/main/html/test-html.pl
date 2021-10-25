#!/usr/bin/perl -T
use strict;
use warnings;
use File::Basename;

$ENV{PATH} = '/bin:/usr/bin';

sub canonpath {
    my ($path) = @_;
    my @c = split m#/+#, $path;
    my @c_new = ();
    for my $component (@c) {
        if ($component eq '.') { next; }
        if ($component eq '..') {
            my $n = $#c_new;
            if ($n >= 0 && $c_new[$n] ne '..') {
                pop @c_new; next;
            }
        }
        push @c_new, $component;
    }
    return join("/", '.', @c_new);
}

my $index_base = File::Basename::dirname(__FILE__);
my $components_path = canonpath($index_base.'/../../components');

require $components_path."/http_html_cmn/html/test-html.pl";
require $components_path."/http_wifi_conf/html/test-html.pl";
require $components_path."/http_clock_conf/html/test-html.pl";
require $components_path."/http_alarm_conf/html/test-html.pl";
require $components_path."/http_firmware/html/test-html.pl";

sub index_handler {
    my ($c, $req) = @_;
    if ($req->method eq 'GET' && $req->uri->path eq '/') {
        return $c->send_file_response($index_base.'/index.html');
    }
    return undef;
}

if ($0 eq __FILE__) {
    add_handlers(\&cmn_handler, \&index_handler);
    add_handlers(\&cmn_handler, \&wifi_conf_handler);
    add_handlers(\&cmn_handler, \&clock_conf_handler);
    add_handlers(\&cmn_handler, \&alarm_conf_handler);
    add_handlers(\&cmn_handler, \&firmware_handler);
    run_httpd(start_httpd(""));
}

1;
__END__

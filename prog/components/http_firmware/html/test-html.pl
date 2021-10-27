#!/usr/bin/perl -T
use strict;
use warnings;
use File::Basename;
use Cwd "cwd";
use JSON;
use Digest::SHA qw(sha256_hex);

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
my $ESP_IMAGE_HEADER_MAGIC = 0xE9;
my $ESP_APP_DESC_MAGIC_WORD = 0xABCD5432;
my $min_image_szie = 0x8000; # arbitrary size
my $partition_size = 0x180000;
my $spiffs_size = 0xf0000;

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
    } elsif ($req->method eq 'GET' && $req->uri->path eq '/firmware/partinfo') {
        my $json = '{"status":1,"appsize":'.$partition_size.',"spiffssize":'.$spiffs_size.'}';
        my $res = json_response($json);
        $c->send_response($res);
        return $res;
    } elsif ($req->method eq 'POST' && $req->uri->path eq '/firmware/update') {
        my $json;
        my $data;
        if (!$req->header('Content-Length')) {
            print "No Content-Length\n";
            $json = '{"status":-1,"message":"Content-Length is required"}';
        } elsif ($req->content_length < $min_image_szie) {
            print "Image is too small: ".$req->content_length."\n";
            $json = '{"status":-1,"message":"Image is too small"}';
        } elsif ($req->content_length > $partition_size) {
            print "Image is too large: ".$req->content_length."\n";
            $json = '{"status":-1,"message":"Image is too large"}';
        } elsif (!$req->header('Content-Type')) {
            print "No Content-Type\n";
            $json = '{"status":-1,"message":"Content-Type is required"}';
        } elsif ($req->header('Content-Type') eq 'application/octet-stream') {
            $data = $req->content;
        } elsif ($req->header('Content-Type') =~ m#^multipart/form-data;(?:.*;)?\s*boundary\s*=\s*(.+)#) {
            $data = value_from_multipart($req, 'firmware');
        } else {
            print "Unsupported Content-Type: ".$req->header('Content-Type')."\n";
            $json = '{"status":-1,"message":"Unsupported Content-Type"}';
        }
        if (defined($data)) {
            my $image_header = length($data) >= 0x18 && substr($data, 0, 0x18);
            my $image_segment_header = length($data) >= 0x20 && substr($data, 0x18, 0x08);
            my $app_desc = length($data) >= 0x120 && substr($data, 0x20, 0x100);
            if (length($data) < 0x120) {
                print "WARN: data is too short\n";
                $json = '{"status":-1,"message":"data is too short"}';
            } elsif (unpack('C', $image_header) != $ESP_IMAGE_HEADER_MAGIC) {
                print "WARN: invalid image header magic\n";
                $json = '{"status":-1,"message":"invalid image header magic"}';
            } elsif (unpack('V', $app_desc) != $ESP_APP_DESC_MAGIC_WORD) {
                print "WARN: invalid app desc magic\n";
                $json = '{"status":-1,"message":"invalid app desc magic"}';
            } else {
                my $version = unpack 'Z32', substr($app_desc, 0x10, 0x20);
                my $project_name = unpack 'Z32', substr($app_desc, 0x30, 0x20);
                my $build_time = unpack 'Z16', substr($app_desc, 0x50, 0x10);
                my $build_date = unpack 'Z16', substr($app_desc, 0x60, 0x10);
                my $idf_ver = unpack 'Z16', substr($app_desc, 0x70, 0x10);
                my $checksum = unpack 'H64', substr($data, length($data) - 32);
                my $calculated_checksum = sha256_hex(substr($data, 0, length($data) - 32));
                print "app_desc: \n";
                print "  version: $version\n";
                print "  project_name: $project_name\n";
                print "  build_time & date: $build_time $build_date\n";
                print "  idf_ver: $idf_ver\n";
                print "size: ".length($data).' / '.$req->content_length."\n";
                print "checksum: $checksum / $calculated_checksum\n";
                if ($checksum ne $calculated_checksum) {
                    $json = '{"status":0,"message":"Checksum does not match"}';
                } else {
                    $json = '{"status":0,"message":"Not implemented"}';
                }
            }
        }
        unless ($json) {
            $json = '{"status":-1,"message":"Bad Requst"}';
        }
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

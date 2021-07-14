#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := wifi-voice-clock
CFLAGS = -Wno-format-truncation

gen-key:
	mkdir -p main/gen
	[ -e main/gen/nvskey.dat ] || dd if=/dev/random of=main/gen/nvskey.dat bs=64 count=1

include $(IDF_PATH)/make/project.mk

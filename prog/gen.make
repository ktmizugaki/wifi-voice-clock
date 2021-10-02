.PHONY: gen-key

gen-key: main/gen/nvskey.dat

main/gen/nvskey.dat:
	mkdir -p main/gen
	[ -e $@ ] || dd if=/dev/random of=$@ bs=64 count=1

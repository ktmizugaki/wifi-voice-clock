.PHONY: all gen-key gen-lang gen-fonts gen-time_vo

all: gen-key gen-lang gen-fonts gen-time_vo

gen-key: main/gen/nvskey.dat

gen-lang: main/gen/lang.h

gen-fonts: main/gen/font_shinonome14.fnt main/gen/font_shinonome12.fnt

gen-time_vo: spiffs/time_vo.bin

main/gen/nvskey.dat:
	mkdir -p main/gen
	[ -e $@ ] || dd if=/dev/random of=$@ bs=64 count=1

main/gen/lang.h: main/lang.txt
	mkdir -p main/gen
	./components/gfx/tools/lang.pl $< $@

main/gen/font_shinonome14.fnt: components/font_shinonome/source/shnmk14.bdf components/font_shinonome/source/shnm7x14r.bdf main/lang.txt
	./components/gfx/tools/bdf2font.pl --in components/font_shinonome/source/shnmk14.bdf --in components/font_shinonome/source/shnm7x14r.bdf --out main/gen/font_shinonome14.fnt --charset=jis --lang main/lang.txt

main/gen/font_shinonome12.fnt: components/font_shinonome/source/shnmk12.bdf components/font_shinonome/source/shnm6x12r.bdf main/lang.txt
	./components/gfx/tools/bdf2font.pl --in components/font_shinonome/source/shnmk12.bdf --in components/font_shinonome/source/shnm6x12r.bdf --out main/gen/font_shinonome12.fnt --charset=jis --lang main/lang.txt

spiffs/time_vo.bin: spiffs/time_vo.txt
	./tools/time_vo.pl convert --in $< --out $@ --dir $$(dirname $@)

.PHONY: test-time_vo
test-time_vo:
	@which play sox >/dev/null || { echo 'This command uses play command from sox' >&2; exit 1; }
	./tools/time_vo.pl playall --in spiffs/time_vo.txt --dir spiffs | bash -ve

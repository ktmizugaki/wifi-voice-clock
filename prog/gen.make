.PHONY: all gen-key gen-lang gen-fonts

all: gen-key gen-lang gen-fonts

gen-key: main/gen/nvskey.dat

gen-lang: main/gen/lang.h

gen-fonts: main/gen/font_shinonome14.fnt main/gen/font_shinonome12.fnt

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

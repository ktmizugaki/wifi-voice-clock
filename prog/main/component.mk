#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

COMPONENT_SRCDIRS = . util
COMPONENT_PRIV_INCLUDEDIRS = util
COMPONENT_EMBED_FILES += gen/nvskey.dat
COMPONENT_EMBED_FILES += gen/font_shinonome14.fnt
COMPONENT_EMBED_FILES += gen/font_shinonome12.fnt
COMPONENT_EMBED_FILES += html/index.html
COMPONENT_EMBED_FILES += data/alarm.wav

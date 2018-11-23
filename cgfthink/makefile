all: cgfthink.dll
objs= cgfthink.obj

cgfthink.dll: $(objs)
	bcc32 -WD -ecgfthink.dll $(objs)

cgfthink.obj: cgfthink.h cgfthink.c
	bcc32 -V0 -Vd -c cgfthink.c

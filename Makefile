CC:=gcc
ifdef WINDIR
    PREFIX:=$(USERPROFILE)
    EXE_EXT:=.exe
else
    PREFIX:=/usr/local
    EXE_EXT:=
endif
BINDIR:=$(PREFIX)/bin

CFLAGS:=-Wall -ansi -pedantic-errors
CP:=cp
INSTALL:=$(CP) -u
MKDIR:=mkdir
OUT:=bin/mmc-docker$(EXE_EXT)
RM:=/bin/rm

.DEFAULT: default

.PHONY: default
default: $(OUT)

.PHONY: clean
clean:
	-$(RM) $(OUT)

.PHONY: install
install: $(OUT)
	@$(MKDIR) -p "$(BINDIR)"
	$(INSTALL) -t "$(BINDIR)" -p $^

$(OUT): src/mmc-docker.c
	@$(MKDIR) -p bin
	$(CC) $(CFLAGS) -o $@ $^

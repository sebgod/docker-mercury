CC:=gcc
ifdef WINDIR
    PREFIX:=$(USERPROFILE)
    EXE_EXT:=.exe
else
    PREFIX:=/usr/local
    EXE_EXT:=
endif
BINDIR:=$(PREFIX)/bin

OUT:=bin/mmc-docker$(EXE_EXT)
CP:=cp
INSTALL:=$(CP) -u
MKDIR:=mkdir

.DEFAULT: default

.PHONY: default
default: $(OUT)

.PHONY: install
install: $(OUT)
	@$(MKDIR) -p "$(BINDIR)"
	$(INSTALL) -t "$(BINDIR)" -p $^

$(OUT): src/mmc-docker.c
	@$(MKDIR) -p bin
	$(CC) -o $@ $^

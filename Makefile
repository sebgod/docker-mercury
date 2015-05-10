CC:=gcc
ifdef WINDIR
    PREFIX:=$(USERPROFILE)
    EXE_EXT:=.exe
else
    PREFIX:=/usr/local
    EXE_EXT:=
endif
BINDIR:=$(PREFIX)/bin

OUT:=bin/mmc-docker
CP:=cp
INSTALL:=$(CP) -u
MKDIR:=mkdir

.DEFAULT: default

.PHONY: default
default: $(OUT)

.PHONY: install
install: $(OUT)$(EXE_EXT)
	@$(MKDIR) -p "$(BINDIR)"
	$(INSTALL) -t "$(BINDIR)" -p $^

$(OUT)$(EXE_EXT): src/mmc-docker.c
	@$(MKDIR) -p bin
	$(CC) -o $@ $^

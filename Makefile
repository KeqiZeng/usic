PREFIX ?= $(HOME)/.cargo
BINDIR ?= $(PREFIX)/bin
CARGO ?= cargo
INSTALL ?= install
UNAME_S := $(shell uname -s)

.PHONY: build install uninstall

build:
	$(CARGO) build --release

install: build
	$(INSTALL) -d "$(BINDIR)"
	$(INSTALL) -m 755 target/release/usic "$(BINDIR)/usic"
ifeq ($(UNAME_S),Darwin)
	$(INSTALL) -m 755 target/release/usic-macos-media "$(BINDIR)/usic-macos-media"
endif

uninstall:
	rm -f "$(BINDIR)/usic"
ifeq ($(UNAME_S),Darwin)
	rm -f "$(BINDIR)/usic-macos-media"
endif

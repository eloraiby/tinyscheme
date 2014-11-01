# GNU Make solution makefile autogenerated by Premake
# Type "make help" for usage help

ifndef config
  config=debug
endif
export config

PROJECTS := tinyscheme

.PHONY: all clean help $(PROJECTS)

all: $(PROJECTS)

tinyscheme: 
	@echo "==== Building tinyscheme ($(config)) ===="
	@${MAKE} --no-print-directory -C . -f tinyscheme.make

clean:
	@${MAKE} --no-print-directory -C . -f tinyscheme.make clean

help:
	@echo "Usage: make [config=name] [target]"
	@echo ""
	@echo "CONFIGURATIONS:"
	@echo "   debug"
	@echo "   release"
	@echo ""
	@echo "TARGETS:"
	@echo "   all (default)"
	@echo "   clean"
	@echo "   tinyscheme"
	@echo ""
	@echo "For more information, see http://industriousone.com/premake/quick-start"

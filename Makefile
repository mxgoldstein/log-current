# Licensed under the MIT License. See LICENSE.
C_FLAGS = -Wall -Wextra -ansi -g -DDEFAULT_LOG_DIR=\"$(DEFAULT_LOG_DIR)\" -DDEFAULT_COMMAND=\"$(DEFAULT_COMMAND)\"
SRC = log-current.c
OUT = log-current

PREFIX=/usr/local

all: $(OUT)
ifndef DEFAULT_LOG_DIR
DEFAULT_LOG_DIR=./
endif

ifndef DEFAULT_COMMAND
DEFAULT_COMMAND=tailf
endif

$(OUT):	
	$(CC) $(C_FLAGS) $(SRC) -o $(OUT)
install: all
	install $(OUT) $(PREFIX)/bin/$(OUT)
uninstall: all
	rm $(PREFIX)/bin/$(OUT)
clean:
	@rm -f $(OUT) || true


.PHONY: all clean install uninstall
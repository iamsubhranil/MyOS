#include "shell.h"
#include <drivers/keyboard.h>
#include <drivers/terminal.h>
#include <sys/string.h>

struct Command {
	const char *command;
	void (*handle)(const char *args);
};

void handle_name(const char *args) {
	(void)args;
	Terminal::info("MyOS");
}

Command commands[] = {{.command = "name", .handle = handle_name}};
bool    runShell   = true;

void Shell::init() {
}

void processBuffer(const char *buffer) {
	for(size_t i = 0; i < sizeof(commands) / sizeof(Command); i++) {
		if(strcmp(buffer, commands[i].command) == 0) {
			commands[i].handle(buffer);
			break;
		}
	}
}

void Shell::run() {
	Terminal::write("\n\n");
	Terminal::write(">> _\b");
	char buffer[1024];
	int  cur = 0;
	while(runShell) {
		char c = Keyboard::getCharacter(true);
		if(c == '\b') {
			Terminal::write(" \b\b_\b");
			cur--;
		} else {
			if(c == '\n') {
				Terminal::write(" ");
			}
			Terminal::write(c);
			if(c != '\n') {
				buffer[cur++] = c;
				Terminal::write("_\b");
			}
		}
		if(c == '\n') {
			buffer[cur] = 0;
			processBuffer(buffer);
			Terminal::write(">> _\b");
			cur = 0;
		}
	}
}

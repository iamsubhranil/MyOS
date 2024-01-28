#include <drivers/keyboard.h>
#include <drivers/terminal.h>
#include <mem/memory.h>
#include <misc/shell.h>
#include <sys/string.h>

void handle_hello(int num) {
	Terminal::info("Hello ", num);
}

Shell::Command *Shell::commands    = NULL;
int             Shell::numCommands = 0;
bool            runShell           = true;

void Shell::init() {
	addCommand("hello", handle_hello);
}

void Shell::processBuffer(const char *buffer, int len) {
	if(len == 0)
		return;
	int cmdLen = 0;
	while(cmdLen < len && buffer[cmdLen] != ' ') cmdLen++;
	for(int i = 0; i < numCommands; i++) {
		if(strncmp(buffer, commands[i].command, cmdLen) == 0) {
			int err = commands[i].handle(&buffer[cmdLen], len - cmdLen,
			                             commands[i].handler);
			if(err == 1) {
				Terminal::prompt(Terminal::Color::Red,
				                 StringSlice(buffer, 0, cmdLen),
				                 "Insufficient arguments!");
			} else if(err == 2) {
				Terminal::prompt(Terminal::Color::Red,
				                 StringSlice(buffer, 0, cmdLen),
				                 "Invalid argument!");
			} else if(err == 3) {
				Terminal::prompt(Terminal::Color::Red,
				                 StringSlice(buffer, 0, cmdLen),
				                 "Incorrect additional arguments!");
			}
			return;
		}
	}
	Terminal::err("Command not found: ", StringSlice(buffer, 0, cmdLen));
}

void Shell::registerCommand(Command c) {
	Command *bak =
	    (Command *)Memory::alloc(sizeof(Command) * (numCommands + 1));
	if(numCommands != 0)
		memcpy(bak, commands, sizeof(Command) * numCommands);
	bak[numCommands]         = c;
	bak[numCommands].command = strdup(c.command);
	numCommands++;
	Memory::free(commands);
	commands = bak;
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
			processBuffer(buffer, cur);
			Terminal::write(">> _\b");
			cur = 0;
		}
	}
}

u32 Shell::Line::dump() const {
	return Terminal::write(StringSlice(source, 0, length));
}

template <> int Shell::Verifier<int>::verify(Line &l) {
	if(l.hasError)
		return 0;
	int         start  = l.current;
	const char *source = l.source;

	while(start < l.length && source[start] == ' ') start++;

	int bak = start;
	while(start < l.length && isnum(source[start])) {
		start++;
	}
	if(start == bak) {
		if(start != l.length)
			l.hasError = 2;
		else
			l.hasError = 1;
		return 0;
	}
	if(source[start] != 0 && source[start] != ' ') {
		l.hasError = 2;
		return 0;
	}
	i32 result = 0;
	for(int i = bak; i < start; i++) {
		result *= 10;
		result += source[i] - '0';
	}
	l.current = start;
	return result;
}

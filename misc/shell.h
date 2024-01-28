#pragma once

#include <sys/myos.h>

struct Shell {
	struct Command {
		const char *command;
		void       *handler;
		int (*handle)(const char *args, int length, void *handler);
	};

	struct Line {
		const char *source;
		int         length;
		int         current;
		int         hasError; // 0 - no error
		                      // 1 - insufficient arguments
		                      // 2 - incorrect argument type
		                      // 3 - extra arguments
		u32 dump() const;
	};

	template <typename T> struct Verifier {
		static bool isnum(char c) {
			return c >= '0' && c <= '9';
		}
		static T verify(Line &source);
	};

	template <typename... T>
	static int execWrapper(Line &l, void *exec, const T &...args) {
		if(l.hasError) {
			return l.hasError;
		} else if(l.current != l.length) {
			l.hasError = 3;
			return l.hasError;
		}
		((void (*)(T...))(exec))(args...);
		return 0;
	}

	static void init();
	static void run();

	static Command *commands;
	static int      numCommands;

	static void processBuffer(const char *buf, int len);
	static void registerCommand(Command c);

	template <typename... T>
	static void addCommand(const char *cmd, void (*exec)(T...)) {
		Command c;
		c.handler = (void *)exec;
		c.command = cmd;
		c.handle  = [](const char *s, int length, void *exec1) {
            Line l = {s, length, 0, 0};
            return execWrapper(l, exec1, Verifier<T>::verify(l)...);
		};
		registerCommand(c);
	}
};

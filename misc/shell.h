#pragma once

struct Shell {
	static void init();
	static void run();

	template <typename T> class Verifier {
		static bool verify(const char *source, int start, int &end, T &result);
	};

	template <> class Verifier<int> {
		static bool isnum(char i) {
			return i >= '0' && i <= '9';
		}

		static bool verify(const char *source, int start, int &end,
		                   int &result) {
			int bak = start;
			while(source[start] != 0 && isnum(source[start])) {
				start++;
			}
			if(start == bak)
				return false;
			if(source[start] != 0 && source[start] != ' ') {
				return false;
			}
			result = 0;
			for(int i = bak; i < start; i++) {
				result *= 10;
				result += source[i] - '0';
			}
			while(source[start] != 0 && source[start] == ' ') start++;
			end = start;
			return true;
		}
	};
};

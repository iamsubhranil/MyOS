#pragma once

template <typename T> struct Option {
	T    value;
	bool has;

	enum Status {
		Nil,
	};

	Option() : has(false) {
	}

	Option(T val) : value(val), has(true) {
	}

	Option(Status s) : Option() {
		(void)s;
	}

	operator bool() {
		return has;
	}

	operator T() {
		return value;
	}

	T get() {
		return value;
	}

	T get(T def) {
		if(has)
			return value;
		return def;
	}
};

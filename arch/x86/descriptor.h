#pragma once

#include <sys/myos.h>

struct Descriptor {
	enum PriviledgeLevel : u8 {
		Ring0 = 0,
		Ring1 = 1,
		Ring2 = 2,
		Ring3 = 3,
	};

	enum Granularity : u8 { Byte = 0, KiloByte = 1 };

	enum OperandSize : u8 { Bits16 = 0, Bits32 = 1 };

	enum Direction : u8 { Up = 0, Down = 1 };

	enum Type : u8 { CodeOrData = 1, System = 0 };

	enum Executable : u8 { Yes = 1, No = 0 };

	enum ReadWrite : u8 {
		Allowed    = 1,
		Disallowed = 0,
	};

	// Flag bytes are in top nibble
	static constexpr u8 flagByte(Granularity g, OperandSize o) {
		return ((u8)g) << 7 | ((u8)o) << 6;
	}

	static constexpr u8 accessByte(PriviledgeLevel p, Type t, Executable e,
	                               Direction d, ReadWrite rw) {
		return (1 << 7)       /* present */
		       | ((u8)p) << 5 /* priviledge */
		       | ((u8)t) << 4 /* type */
		       | ((u8)e) << 3 /* ex */
		       | ((u8)d) << 2 /* direction or conform */
		       | ((u8)rw) << 1 /* rw bit */;
	}
};

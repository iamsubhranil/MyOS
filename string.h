#pragma once

#include "myos.h"

siz strlen(const char *str) {
	siz len = 0;
	while(str[len]) len++;
	return len;
}

#include <stdint.h>
#include <stdio.h>

#ifndef LargeNumbers_H
	#include "LargeNumbers.h"
#endif

void test2() {
	LargeNumberResult a, b;
	LargeNumberFromDec("3456783450687340586734058963048567304859763049856304895603895603859734098576340985763409853408953", 500, 0, &a);
	LargeNumberFromDec("23485792837485984783928378475894878398728398478574839287459287349582734985", 500, 1, &b);

	if (a.error || b.error) {
		printf("Received error result.");
	} else {
		char aBuf[1000];
		char bBuf[1000];
		char sBuf[1000];

		LargeNumberResult dif;

		LargeNumberToString(a.result, aBuf, 1000);
		LargeNumberToString(b.result, bBuf, 1000);

		LargeNumberAdd(a.result, b.result, &dif);

		LargeNumberToString(dif.result, sBuf, 1000);

		printf("%s + %s = %s\n", aBuf, bBuf, sBuf);
		printf("a size: %lu, b size: %lu, d size: %lu\n", a.result->size, b.result->size, dif.result->size);
		LargeNumberFree(a.result);
		LargeNumberFree(b.result);
		LargeNumberFree(dif.result);
	}
}

int main(int argc, char** argv) {
	test2();

	return 0;
}

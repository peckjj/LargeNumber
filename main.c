#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifndef LargeNumbers_H
	#include "LargeNumbers.h"
#endif

int main(int argc, char** argv) {
	if (argc < 2) {
		return 0;
	}

	char buf[10000];

	LargeNumberResult* res = malloc(sizeof(LargeNumberResult));

	printf("Reading as %s (%lu digits)...\n", "dec", strlen(argv[1]));

	LargeNumberFromDec(argv[1], 9999, 0, res);

	if (res->error) {
		printf("Parse error.\n");
		return -1;
	}

	memset(buf, '\0', 10000);

	printf("As decimal:\n");
	LargeNumberToString(res->result, 10, buf, 10000);

	printf("Actual -> %s (Length=%lu)\n", buf, strlen(buf));
	printf("Test   -> %s\n", argv[1]);
	printf("%s\n", strcmp(buf, argv[1]) ? "Fail." : "Pass!");

	memset(buf, '\0', 10000);
	LargeNumberToStringHex(res->result, buf, 10000);

	printf("As hex:\n%s\n(Length=%lu)\n", buf, strlen(buf));
	printf("Total size of number = %lu B (%lu b)\n", res->result->size * 8, res->result->size * 8 * 8);

	return 0;
}

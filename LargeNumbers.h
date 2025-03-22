#ifndef LargeNumbers_H
	#define LargeNumbers_H 1
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define CHUNK_MAX (uint64_t)0xFFFFFFFFFFFFFFFF

enum LargeNumberError {
	SUCCESS,
	PARSE_INVALID_CHAR,
	PARSE_EMPTY_STR,
	LARGENUMBER_ERROR_COUNT
};

/*
char* StringLargeNumberError[] = {
	"Operation completed successfully",
	"Invalid hexidecimal character encountered while parsing",
	"Not an error"
};
*/

typedef struct LargeNumber {
	uint64_t *data;
	size_t size;
	uint8_t sign;
} LargeNumber;

typedef struct LargeNumberResult {
	LargeNumber* result;
	int error;
} LargeNumberResult;

/**
	Convert a hexidecimal string to a LargeNumber. Automatically handles
	memory allocation. Parsing ends when the length of *hex* provided by *len*
	is reached, or when the \0 NULL terminating character is encountered, whichever
	occurs first.

	hex - Hexidecimal string representation of large number

	len - Length of hexidecimal string (if there is no \0 terminating character)

	sign - Represents sign (+/-) of large number. A non-zero value indicates the large number is negative (< 0)

	result - pointer to LargeNumberResult to store parse result

		Returns 0 on success, otherwise returns non-zero and sets result->error field
*/
int LargeNumberFromDec(char* dec, size_t len, uint8_t sign, LargeNumberResult* result);
int LargeNumberFromHex(char* hex, size_t len, uint8_t sign, LargeNumberResult* result);
void LargeNumberToString(LargeNumber* num, char* buf, size_t n);
void LargeNumberFree(LargeNumber* num);

int8_t LargeNumberCmp(LargeNumber* a, LargeNumber* b);
void LargeNumberAdd(LargeNumber* a, LargeNumber* b, LargeNumberResult* result);
void LargeNumberSub(LargeNumber* a, LargeNumber* b, LargeNumberResult* result);

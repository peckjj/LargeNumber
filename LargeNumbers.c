#ifndef LargeNumbers_H
	#include "LargeNumbers.h"
#endif

#include <stdio.h>

uint8_t isHex(char c) {
	if (
		c < '0' ||
		(c > '9' && c < 'A') ||
		(c > 'F' && c < 'a') ||
		c > 'f'
	) {
		return 0;
	}

	return 1;
}

uint8_t hexToByte(char c) {
	switch (c) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			return c - '0';
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
			return c - 'A' + 10;
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
		default:
			return c - 'a' + 10;
	}
}

void ShrinkLargeNumber(LargeNumber* num) {
	size_t finalSize = num->size;
	while (!(num->data[finalSize - 1])) {
		finalSize--;
	}
	if (finalSize < num->size) {
		num->data = realloc(num->data, finalSize * sizeof(uint64_t));
		num->size = finalSize;
	}
}

int LargeNumberFromDec(char* dec_original, size_t len, uint8_t sign, LargeNumberResult* result) {
	result->result = NULL;
	for (size_t i = 0; i < len; i++) {
		if (dec_original[i] == '\0') {
			len = i;
			break;
		}

		if (dec_original[i] < '0' || dec_original[i] > '9') {
			result->error = PARSE_INVALID_CHAR;

			return -1;
		}
	}

	if (!len) {
		result->error = PARSE_EMPTY_STR;
		return - 1;
	}

	// Copy dec_original to another buffer, could be from a statically defined string
	char* dec = calloc(len + 1, sizeof(char));
	strncpy(dec, dec_original, len + 1);

	size_t bitsRequired = len * 4;
	size_t intsRequired = (bitsRequired / 64) + 1;

	result->result = malloc(sizeof(LargeNumber));
	result->result->size = intsRequired;
	result->result->sign = sign;
	result->result->data = calloc(intsRequired, sizeof(uint64_t));

	// convert all decimal chars to actual numbers. More costly compute-wise, but easier to reason.
	for (size_t i = 0; i < len; i++) {
		dec[i] -= '0';
	}

	// Continue to divide by 2. The last decimal digit MOD 2 determines the next bit.
	size_t curBit = 0;
	size_t curChunk = 0;
	uint8_t carry = 0;
	uint8_t firstNonZero = 0;
	size_t startIdx = 0; // start at first idx of dec string. Use this as terminating condition
	while (startIdx < len) {
		carry = 0;
		result->result->data[curChunk] += ( (uint64_t)(dec[len - 1] % 2) << curBit);

		for (size_t i = startIdx; i < len; i++) {
			if ( !firstNonZero && !(dec[i]) ) {
				startIdx++;
				continue;
			}

			firstNonZero = 1;
			dec[i] += (10 * carry);
			carry = dec[i] % 2;
			dec[i] /= 2;
		}

		if (curBit == 63) {
			curBit = 0;
			curChunk++;
		} else {
			curBit++;
		}
		firstNonZero = 0;
	}

	free(dec);

	ShrinkLargeNumber(result->result);
	result->error = SUCCESS;
	return 0;
}

int LargeNumberFromHex(char* hex, size_t len, uint8_t sign, LargeNumberResult* result) {
	result->result = NULL;

	// Check if string is valid
	for (size_t i = 0; i < len; i++) {
		if (hex[i] == '\0') {
			len = i;
			break;
		}

		if (!isHex(hex[i])) {
			result->error = PARSE_INVALID_CHAR;

			return -1;
		}
	}

	if (!len) {
		result->error = PARSE_EMPTY_STR;
		return - 1;
	}

	size_t intsRequired = (len / 16);
	if (len %16) {
		intsRequired++;
	}

	result->result = malloc(sizeof(LargeNumber));
	result->result->data = malloc(intsRequired * sizeof(uint64_t));
	memset(result->result->data, 0, sizeof(uint64_t) * intsRequired);

	result->result->size = intsRequired;
	result->result->sign = sign;

	int dataIdx = 0;
	int shiftCounter = 0;
	int curIdx = 0;

	while (curIdx < len) {
		uint8_t isSecondDigit = curIdx % 2;
		uint8_t curCharVal = hexToByte(hex[len - curIdx - 1]);
		if (isSecondDigit) {
			curCharVal = curCharVal << 4;
		}

		result->result->data[dataIdx] += ( ((uint64_t)curCharVal) << (shiftCounter * 8) );

		if (isSecondDigit) {
			shiftCounter++;
		}

		if (curIdx % 16 == 15) {
			shiftCounter = 0;
			dataIdx++;
		}

		curIdx++;
	}

	result->error = SUCCESS;
	return 0;
}

void LargeNumberToStringHex(LargeNumber* num, char* buf, size_t n) {
	size_t curPos = 0;

	while (curPos < num->size && snprintf(buf, n, "%016lX ", num->data[curPos]) < n) {
		n -= 17;
		buf += 17;
		curPos++;
	}
}

int LargeNumberToString(LargeNumber* num, uint8_t base, char* buf, size_t n) {
	if(n <  1) {
		return SUCCESS;
	}

	if (base < 2 || base > 10) {
		return INVALID_BASE;
	}

	LargeNumberResult* cloneRes = malloc(sizeof(LargeNumberResult));
	LargeNumberCpy(num, cloneRes);

	if (cloneRes->result == NULL) {
		free(cloneRes);
		return LARGE_NUM_NULL;
	}

	LargeNumber* cpy = cloneRes->result;

	char* tmp = calloc( (num->size * 64) + 2, 1 );
	size_t outputIdx = (num->size * 64) + 1;
	tmp[outputIdx--] = '\0';
	uint64_t c = 0;
	uint64_t b = 0;
	uint64_t b_h = 0;

	size_t chunkIdx = 0;

	uint64_t T = 0xFFFFFFFFFFFFFFFF;

	while (LargeNumberCmpZ(cpy)) {
		chunkIdx = cpy->size - 1;
		while (!cpy->data[chunkIdx]) { 
			chunkIdx--;
		}

		c = 0;

		for (; chunkIdx < cpy->size; chunkIdx--) {
			b = cpy->data[chunkIdx];

			b_h = ((T / base) * c) + (((T % base + 1)*c + (b % base)) / base) + (b / base);

			c = ( (((((T % base) + 1) % base) * c) % base) + (b % base) ) % base;

			cpy->data[chunkIdx] = b_h;
		}

			tmp[outputIdx--] = c + '0';
	}

	// copy tmp in buf in reverse order
	char* start = &(tmp[outputIdx + 1]);

	if (cpy->sign) {
		snprintf(buf, n, "-%s", start);
	} else {
		snprintf(buf, n, "%s", start);
	}

	LargeNumberFree(cpy);
	free(cloneRes);
	free(tmp);

	return SUCCESS;
}

void LargeNumberCpy(LargeNumber* num, LargeNumberResult* result) {
	result->result = NULL;

	if (num == NULL) {
		return;
	}

	result->result = malloc(sizeof(LargeNumber));
	result->result->data = malloc(sizeof(uint64_t) * num->size);

	memcpy(result->result->data, num->data, sizeof(uint64_t) * num->size);
	result->result->size = num->size;
	result->result->sign = num->sign;
}

LargeNumber* LargeNumberDigit(size_t magnitude) {
	LargeNumber* num = malloc( sizeof(LargeNumber) );

	size_t intsNeeded = (magnitude / 64) + 1;
	uint8_t digit = magnitude % 64;

	num->size = intsNeeded;
	num->data = calloc(num->size, sizeof(uint64_t));
	num->sign = 0;

	num->data[num->size - 1] = ((uint64_t)1) << digit;

	ShrinkLargeNumber(num);
	return num;
}

void LargeNumberFree(LargeNumber* num) {
	free(num->data);
	free(num);
	num = NULL;
}

int8_t LargeNumberCmpZ(LargeNumber* a) {
	if (a == NULL || a->data == NULL) {
		return 0;
	}

	for (size_t i = 0; i < a->size; i++) {
		if (a->data[i]) {
			return a->sign ? -1: 1;
		}
	}

	return 0;
}

int8_t LargeNumberCmp(LargeNumber* a, LargeNumber* b) {
	uint8_t aIsZero = 1;
	uint8_t bIsZero = 1;
	for (size_t i = 0; i < a->size; i++) {
		if (a->data[i]) {
			aIsZero = 0;
			break;
		}
	}
	for (size_t i = 0; i < b->size; i++) {
		if (b->data[i]) {
			bIsZero = 0;
			break;
		}
	}

	if (aIsZero && bIsZero) {
		return 0;
	}

	if (a->sign != b->sign) {
		return a->sign ? -1 : 1;
	}

	if (a->size > b->size) {
		return a->sign ? -1 : 1;
	} else if (b->size > a->size) {
		return a->sign ? 1 : -1;
	}

	for (size_t i = 0; i < a->size; i++) {
		uint64_t aChunk = a->data[a->size - (i + 1)];
		uint64_t bChunk = b->data[b->size - (i + 1)];

		if (aChunk == bChunk) {
			continue;
		}

		if (aChunk < bChunk) {
			return a->sign ? 1 : -1;
		} else if (bChunk < aChunk) {
			return a->sign ? -1 : 1;
		}
	}

	return 0;
}

void LargeNumberAdd(LargeNumber* a, LargeNumber* b, LargeNumberResult* result) {
	result->result = NULL;

	uint8_t aIsNegative = a->sign != 0;
	uint8_t bIsNegative = b->sign != 0;

	LargeNumber a_copy = *a;
	LargeNumber b_copy = *b;
	a_copy.sign = 0;
	b_copy.sign = 0;
	if (aIsNegative && !bIsNegative) {
		LargeNumberSub(&b_copy, &a_copy, result);
		return;
	}
	if (!aIsNegative && bIsNegative) {
		LargeNumberSub(&a_copy, &b_copy, result);
		return;
	}

	uint8_t sign = aIsNegative && bIsNegative;

	// Inputs have been manipulated so that we can now perform a + b such that both are positive.

	// Maximum size is the bit size of the largest number + 1. Therefore, just add another uint64_t to
	// account for overflow, and reallocate if not needed.
	size_t intsRequired = a->size;
	if (a->size < b->size) {
		intsRequired = b->size;
	}
	intsRequired++;

	result->result = malloc(sizeof(LargeNumber));
	result->result->data = calloc(intsRequired, sizeof(uint64_t));
	result->result->size = intsRequired;

	// Elementary my dear Watson. Iterate through all the bytes of both numbers. First, add 1 to
	// the result LargeNumber if there was overflow in the previous iteration. Then, reset that 
	// flag is there will be overflow for this iteration. Finally, add both a and b to the result.
	uint8_t overflow = 0;
	for (size_t i = 0; i < intsRequired; i++) {
		result->result->data[i] += overflow;
		if (i < a->size && i < b->size) {
			result->result->data[i] += a->data[i] + b->data[i];
			overflow = a->data[i] > (CHUNK_MAX - b->data[i]);
		} else if (i < a->size) {
			result->result->data[i] += a->data[i];
			overflow = 0;
		} else if (i < b->size) {
			result->result->data[i] += b->data[i];
			overflow = 0;
		} else {
			break;
		}
	}

	ShrinkLargeNumber(result->result);

	result->result->sign = sign;
	result->error = SUCCESS;
}

void LargeNumberSub(LargeNumber* a, LargeNumber* b, LargeNumberResult* result) {
	result->result = NULL;

	uint8_t aIsNegative = a->sign != 0;
	uint8_t bIsNegative = b->sign != 0;

	LargeNumber a_copy = *a;
	LargeNumber b_copy = *b;
	a_copy.sign = 0;
	b_copy.sign = 0;
	if (aIsNegative && !bIsNegative) {
		LargeNumberAdd(&a_copy, &b_copy, result);
		result->result->sign = 1;
		return;
	}
	if (!aIsNegative && bIsNegative) {
		LargeNumberAdd(&a_copy, &b_copy, result);
		return;
	}

	if (LargeNumberCmp(&a_copy, &b_copy) < 0) {
		LargeNumberSub(&b_copy, &a_copy, result);
		result->result->sign = 1;
		return;
	}

	uint8_t sign = aIsNegative && bIsNegative;

	// Inputs have been manipulated so that we can now perform a - b such that both are positive,
	// and a >= b;

	// Maximum size can be no larger than the largest LargeNumber, a. Reallocate for empty bytes
	// if not needed
	size_t intsRequired = a->size;
	result->result = malloc(sizeof(LargeNumber));
	result->result->data = calloc(intsRequired, sizeof(LargeNumber));
	result->result->size = intsRequired;

	// Similar to add, iterate through each chunk of a and b. First, assign the result's chunk to a's.
	uint8_t underflow = 0;
	for (size_t i = 0; i < a->size; i++) {
		result->result->data[i] = a->data[i];
		result->result->data[i] -= underflow;

		if (i >= b->size) {
			underflow = 0;
			continue;
		}

		if ((result->result->data[i] == CHUNK_MAX && underflow) || result->result->data[i] < b->data[i]) {
			underflow = 1;
		} else {
			underflow = 0;
		}

		result->result->data[i] -= b->data[i];
	}

	ShrinkLargeNumber(result->result);

	result->result->sign = sign;
	result->error = SUCCESS;
}
/*
void LargeNumberMult(LargeNumber* a, LargeNumber* b, LargeNumberResult* result) {


}
*/

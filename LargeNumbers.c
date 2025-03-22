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

void LargeNumberToString(LargeNumber* num, char* buf, size_t n) {
	if (!n) {
		return;
	}
	// We need this much space at most to hold the base 10 number. This is derived
	// via the following:

	// If our number contains Z bits (digits), then the number < 2^Z
	// The number of digits in base 10 which are required can be described as:
	//
	//	log_10(2^Z)
	//
	// By the formula of base-conversion with logarithms, we get the following, simplified in steps:
	//	1. log_10(2^Z) = log_2(2^Z) / log_2(10)
	//	2. log_10(2^Z) = Z / log_2(10)
	//	3. Number of base 10 digits = Z / ~3.3
	//
	// Therefore, if we divide the number of digits in the LargeNumber by 3 (an over-correction) and add 2
	// (1 to account for the minus sign, another to respect the fact that we may be wrong), then this should
	// be enough space to perform the calc.
	size_t spaceRequired = ((num->size * 64) / 3) + 2;

	char* decimal = calloc(spaceRequired, 1);
	char* decimal_temp = calloc(spaceRequired, 1);

	// `decimal` will hold the final result, which we will use to copy into `buf`. `decimal_temp`
	// will be used to calculate each power of 2 as we encounter them in the LargeNumber, before
	// it is added to `decimal`

	// For each 64-bit int
	for (int curChunk = 0; curChunk < num->size; curChunk++) {
		// For each bit
		for (int curBit = 0; curBit < 64; curBit++) {
		// If bit is on, put '1' in the first index of `decimal_temp`
			if ( num->data[curChunk] & ((uint64_t)(1) << curBit) ) {
				decimal_temp[0] = 1;
				// for every power of that digit
				size_t pow = (curChunk * 64) + curBit;
				while (pow > 0) {
					// multiple `decimal_temp` by 2
					uint8_t carry = 0;
					for (size_t curDigit = 0; curDigit < spaceRequired; curDigit++) {
						decimal_temp[curDigit] *= 2;
						decimal_temp[curDigit] += carry;
						carry = decimal_temp[curDigit] / 10;
						decimal_temp[curDigit] %= 10;
					}
					pow--;
				}
				// Add `decimal_temp` to `decimal`
				uint8_t carry = 0;
				for (size_t curDigit = 0; curDigit < spaceRequired; curDigit++) {
					decimal[curDigit] += decimal_temp[curDigit] + carry;
					carry = decimal[curDigit] / 10;
					decimal[curDigit] %= 10;
				}
				// Zero `decimal_temp`
				memset(decimal_temp, 0, spaceRequired);
			}
		}
	}

	// copy `decimal` to `buf`
	uint8_t firstNonZero = 0;
	size_t outputIdx = 0;

	for (size_t digit = 0; digit < spaceRequired && outputIdx < n; digit++) {
		if (digit == 0 && num->sign) {
			buf[outputIdx] = '-';
			outputIdx++;
			continue;
		}

		char curDigitValue = decimal[spaceRequired - (digit + 1)];
		if (curDigitValue == 0 && !firstNonZero) {
			continue;
		}

		firstNonZero = 1;
		buf[outputIdx++] = curDigitValue + '0';
	}

	if (!firstNonZero) {
		buf[0] = '0';
		outputIdx++;
	}

	size_t lastIdx = outputIdx >= n ? (n - 1) : outputIdx;

	buf[lastIdx] = '\0';

	free(decimal);
	free(decimal_temp);
}

void LargeNumberFree(LargeNumber* num) {
	free(num->data);
	free(num);
	num = NULL;
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

#include "cdma.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BIT_LENGTH 8
#define CODE_LENGTH sizeof(char) * BIT_LENGTH

struct cdma_base_station
{
	int* walsh_codes;
	size_t walsh_codes_size;
};

struct cdma_transmitter
{
	cdma_base_station_t* base_station;
	int** codes;
	size_t codes_size;
	size_t prev_message_size;
};

struct cdma_receiver
{
	cdma_base_station_t* base_station;
};

struct cdma_encoded_message
{
	int* encoded;
	size_t size;
};

static unsigned int get_log2(unsigned int n)
{
	unsigned int p = 0;

	while (n > 0)
	{
		n >>= 1;
		p++;
	}

	return p;
}

static void
	walsh_codes_init(unsigned int k, cdma_base_station_t* base_station, size_t x, size_t y, size_t local_size, int sign)
{
	if (k == 0)
	{
		// H(1) = [1]
		base_station->walsh_codes[y * base_station->walsh_codes_size + x] = sign;
	}
	else if (k == 1)
	{
		// H(2) = [1  1]
		//        [1 -1]
		base_station->walsh_codes[y * base_station->walsh_codes_size + x] = sign;
		base_station->walsh_codes[y * base_station->walsh_codes_size + (x + 1)] = sign;
		base_station->walsh_codes[(y + 1) * base_station->walsh_codes_size + x] = sign;
		base_station->walsh_codes[(y + 1) * base_station->walsh_codes_size + (x + 1)] = -sign;
	}
	else
	{
		// H(n) = [H(n - 1)  H(n - 1)]
		//        [H(n - 1) -H(n - 1)]
		const unsigned next = k - 1;
		const size_t step = local_size >> 1;
		walsh_codes_init(next, base_station, x, y, step, sign);
		walsh_codes_init(next, base_station, x + step, y, step, sign);
		walsh_codes_init(next, base_station, x, y + step, step, sign);
		walsh_codes_init(next, base_station, x + step, y + step, step, -sign);
	}
}

cdma_base_station_t* cdma_base_station_create(unsigned int n, cdma_errno_t* error_code)
{
	cdma_base_station_t* base_station = NULL;

	if ((n & (n - 1)) != 0)
	{
		if (error_code != NULL)
		{
			*error_code = CDMA_INVALID_ARGUMENT;
		}
		goto l_fail;
	}

	base_station = malloc(sizeof(struct cdma_base_station));
	if (base_station == NULL)
	{
		if (error_code)
		{
			*error_code = CDMA_OUT_OF_MEMORY;
		}
		goto l_fail;
	}

	base_station->walsh_codes_size = n;
	base_station->walsh_codes =
		malloc(sizeof(int) * (base_station->walsh_codes_size * base_station->walsh_codes_size));

	if (base_station->walsh_codes == NULL)
	{
		if (error_code)
		{
			*error_code = CDMA_OUT_OF_MEMORY;
		}
		free(base_station);
		goto l_fail;
	}

	const unsigned int k = get_log2(n);
	walsh_codes_init(k - 1, base_station, 0, 0, base_station->walsh_codes_size, 1);

#ifdef CDMA_DEBUG
	const size_t w = base_station->walsh_codes_size;
	printf("[DEBUG] Base station: Walsh codes for n = %u\n", n);
	for (size_t i = 0; i < w; i++)
	{
		printf("[DEBUG] Base station: ");
		for (size_t j = 0; j < w; j++)
		{
			printf("%i ", base_station->walsh_codes[i * w + j]);
		}
		printf("\n");
	}
#endif

	return base_station;

l_fail:
	return NULL;
}

void cdma_base_station_destroy(cdma_base_station_t* base_station)
{
	if (base_station != NULL)
	{
		free(base_station->walsh_codes);
	}
	free(base_station);
}

cdma_transmitter_t*
	cdma_transmitter_create(cdma_base_station_t* base_station, cdma_errno_t* error_code)
{
	cdma_transmitter_t* transmitter = NULL;

	if (base_station == NULL)
	{
		if (error_code != NULL)
		{
			*error_code = CDMA_INVALID_ARGUMENT;
		}
		goto l_fail;
	}

	transmitter = malloc(sizeof(struct cdma_transmitter));

	if (transmitter == NULL)
	{
		if (error_code != NULL)
		{
			*error_code = CDMA_OUT_OF_MEMORY;
		}
		goto l_fail;
	}

	transmitter->base_station = base_station;
	transmitter->codes_size = 0;
	transmitter->prev_message_size = 0;
	transmitter->codes = calloc(base_station->walsh_codes_size, sizeof(int*));

	if (transmitter->codes == NULL)
	{
		if (error_code != NULL)
		{
			*error_code = CDMA_OUT_OF_MEMORY;
		}
		free(transmitter);
		goto l_fail;
	}

	return transmitter;

l_fail:
	return NULL;
}

cdma_errno_t cdma_transmitter_add_station(cdma_transmitter_t* transmitter, const char* message)
{
	cdma_errno_t error_code = CDMA_SUCCESS;

	if (transmitter == NULL || message == NULL)
	{
		error_code = CDMA_INVALID_ARGUMENT;
		goto l_end;
	}

	const size_t message_size = strlen(message);
	if (transmitter->prev_message_size == 0)
	{
		transmitter->prev_message_size = message_size;
	}
	else if (transmitter->prev_message_size != message_size)
	{
		error_code = CDMA_BAD_MESSAGE_SIZE;
		goto l_end;
	}

	const size_t code_size = CODE_LENGTH * message_size;
	int* code = malloc(sizeof(int) * code_size);

	if (code == NULL)
	{
		error_code = CDMA_OUT_OF_MEMORY;
		goto l_end;
	}

	size_t index = 0;
	for (size_t i = 0; i < message_size; i++)
	{
		const char c = message[i];
		for (int j = 0; j < CODE_LENGTH; j++)
		{
			const int shift = (CODE_LENGTH - 1 - j);
			const int mask = 1 << shift;
			const int x = (c & mask) >> shift;
			code[index++] = (x > 0 ? 1 : -1);
		}
	}

	transmitter->codes[transmitter->codes_size++] = code;

#ifdef CDMA_DEBUG
	printf("[DEBUG] Transmitter: Code for '%s' = ", message);
	for (size_t i = 0; i < index; i++)
	{
		printf("%i ", code[i]);
	}
	printf("\n");
#endif

l_end:
	return error_code;
}

static int* get_encoded_code(const int* code, const int* walsh_code, size_t walsh_code_size, size_t code_size)
{
	int* encoded = NULL;
	const size_t size = walsh_code_size * code_size;

	encoded = malloc(sizeof(int) * size);
	if (encoded == NULL)
	{
		goto l_end;
	}

	// (c0, c1, c2, c3)
	//                                      *
	// (w0, w1, w2, w3)
	//                                      =
	// (w0 * c0, w1 * c0, w2 * c0, w3 * c0,
	//  w0 * c1, w1 * c1, w2 * c1, w3 * c1,
	//  w0 * c2, w2 * c2, w3 * c2, w3 * c2,
	//  w0 * c3, w2 * c3, w3 * c3, w3 * c3)
	//                                      =
	// (s0,  s1,  s2,  s3,  s4,
	//  s5,  s6,  s7,  s8,  s9,
	//  s10, s11, s12, s13, s14,
	//  s15, s16)
	size_t index = 0;
	for (size_t i = 0; i < code_size; i++)
	{
		const int sign = code[i];
		for (size_t j = 0; j < walsh_code_size; j++)
		{
			encoded[index++] = sign * walsh_code[j];
		}
	}

l_end:
	return encoded;
}

cdma_encoded_message_t*
	cdma_transmitter_send(const cdma_transmitter_t* transmitter, cdma_errno_t* error_code)
{
	cdma_encoded_message_t* encoded_message = NULL;

	if (transmitter == NULL)
	{
		if (error_code != NULL)
		{
			*error_code = CDMA_INVALID_ARGUMENT;
		}
		goto l_fail;
	}

	encoded_message = malloc(sizeof(struct cdma_encoded_message));

	if (encoded_message == NULL)
	{
		if (error_code != NULL)
		{
			*error_code = CDMA_OUT_OF_MEMORY;
		}
		goto l_fail;
	}

	// Size in bits of single message.
	const size_t code_size = transmitter->prev_message_size * CODE_LENGTH;
	// Total encoded message size.
	const size_t size = code_size * transmitter->base_station->walsh_codes_size;

	encoded_message->size = size;
	encoded_message->encoded = calloc(size, sizeof(int));

	if (encoded_message->encoded == NULL)
	{
		if (error_code != NULL)
		{
			*error_code = CDMA_OUT_OF_MEMORY;
		}
		free(encoded_message);
		goto l_fail;
	}

	for (size_t i = 0; i < transmitter->codes_size; i++)
	{
		// Code from station.
		const int* code = transmitter->codes[i];
		// Walsh single code size.
		const size_t walsh_code_size = transmitter->base_station->walsh_codes_size;
		const size_t walsh_codes_shift = walsh_code_size * i;
		// Walsh code of station.
		const int* walsh_code = transmitter->base_station->walsh_codes + walsh_codes_shift;
		int* encoded = get_encoded_code(code, walsh_code, walsh_code_size, code_size);

		if (encoded == NULL)
		{
			if (error_code != NULL)
			{
				*error_code = CDMA_OUT_OF_MEMORY;
			}
			cdma_encoded_message_destroy(encoded_message);
			goto l_fail;
		}

#ifdef CDMA_DEBUG
		printf("[DEBUG] Transmitter: Encoding signal #%zu = ", i);
		for (size_t j = 0; j < code_size; j++)
		{
			printf("%i ", code[i]);
		}
		printf("\n[DEBUG] Transmitter: Encoded signal #%zu = ", i);
		for (size_t j = 0; j < size; j++)
		{
			printf("%i ", encoded[j]);
		}
		printf("\n");
#endif

		// (s0, s1, s2, s3, s4, s5, ...)
		//                               +
		// (s0, s1, s2, s3, s4, s5, ...)
		//                               +
		// . . . . . . . . . . . . . . .
		//                               =
		// (e0, e1, e2, e3, e4, e5, ...)
		for (size_t j = 0; j < size; j++)
		{
			encoded_message->encoded[j] += encoded[j];
		}

		free(encoded);
	}

#ifdef CDMA_DEBUG
	printf("[DEBUG] Transmitter: Encoded data = ");
	for (size_t i = 0; i < size; i++)
	{
		printf("%i ", encoded_message->encoded[i]);
	}
	printf("\n");
#endif

	return encoded_message;

l_fail:
	return NULL;
}

void cdma_transmitter_destroy(cdma_transmitter_t* transmitter)
{
	if (transmitter)
	{
		if (transmitter->codes != NULL)
		{
			for (size_t i = 0; i < transmitter->codes_size; i++)
			{
				free(transmitter->codes[i]);
			}
			free(transmitter->codes);
		}
	}
	free(transmitter);
}

cdma_receiver_t* cdma_receiver_create(cdma_base_station_t* base_station, cdma_errno_t* error_code)
{
	cdma_receiver_t* receiver = NULL;

	if (base_station == NULL)
	{
		if (error_code != NULL)
		{
			*error_code = CDMA_INVALID_ARGUMENT;
		}
		goto l_fail;
	}

	receiver = malloc(sizeof(struct cdma_receiver));

	if (receiver == NULL)
	{
		if (error_code != NULL)
		{
			*error_code = CDMA_OUT_OF_MEMORY;
		}
		goto l_fail;
	}

	receiver->base_station = base_station;

	return receiver;

l_fail:
	return NULL;
}

char* cdma_receiver_get(
	const cdma_receiver_t* receiver,
	const cdma_encoded_message_t* encoded_message,
	size_t station,
	cdma_errno_t* error_code)
{
	char* message = NULL;

	if (receiver == NULL || encoded_message == NULL)
	{
		if (error_code != NULL)
		{
			*error_code = CDMA_INVALID_ARGUMENT;
		}
		goto l_fail;
	}

	// Size of one Walsh code.
	const size_t walsh_code_size = receiver->base_station->walsh_codes_size;
	// Encoded code size for one character.
	const size_t single_code_size = CODE_LENGTH * walsh_code_size;
	// Total message size.
	const size_t message_size = encoded_message->size / single_code_size;

#ifdef CDMA_DEBUG
	printf("[DEBUG] Receiver: Message size = %zu\n", message_size);
#endif

	message = calloc(message_size + 1, sizeof(char));

	if (message == NULL)
	{
		if (error_code != NULL)
		{
			*error_code = CDMA_OUT_OF_MEMORY;
		}
		goto l_fail;
	}

	const size_t walsh_code_shift = walsh_code_size * station;
	const int* walsh_code = receiver->base_station->walsh_codes + walsh_code_shift;

#ifdef CDMA_DEBUG
	printf("[DEBUG] Receiver: Walsh code for station #%zu = ", station);
	for (size_t i = 0; i < walsh_code_size; i++)
	{
		printf("%i ", walsh_code[i]);
	}
	printf("\n");
#endif

	size_t index = 0;
	for (size_t i = 0; i < message_size; i++)
	{
		int data[CODE_LENGTH] = { 0 };

		// (d0, d1, d2, d3, d4, d5, d6, d7) * (c0, c1)
		// ((d0, d1), (d2, d3), (d4, d5), (d6, d7)) * (c0, c1)
		// ((d0 * c0 + d1 * c2),
		//  (d2 * c0 + d3 * c1),
		//  (d4 * c0 + d5 * c1),
		// 	(d6 * c0 + d7 * c1)
		// )
		for (size_t j = 0; j < CODE_LENGTH; j++)
		{
			int sum = 0;

#ifdef CDMA_DEBUG
			printf("[DEBUG] Receiver: Decoding data = ");
#endif

			for (size_t k = 0; k < CODE_LENGTH; k++)
			{
#ifdef CDMA_DEBUG
				printf("%i ", encoded_message->encoded[index]);
#endif
				const int a = encoded_message->encoded[index++];
				const int b = walsh_code[k];
				sum += (a * b);
			}

#ifdef CDMA_DEBUG
			printf("\n");
#endif

			data[j] = sum;
		}

#ifdef CDMA_DEBUG
		printf("[DEBUG] Receiver: Decoded character #%zu = ", i);
		for (size_t j = 0; j < CODE_LENGTH; j++)
		{
			printf("%i ", data[j]);
		}
		printf("\n");
#endif

		// (e0, e1, e2, e3, e4, e5, e6, e7)
		// forall i, bi = 1 if ei > 0
		// (b0, b1, b2, b3, b4, b5, b6)
		// char = b0 << 7
		//      | b1 << 6
		//      | b2 << 5
		//      | b3 << 4
		//      | b4 << 3
		//      | b5 << 2
		//      | b6 << 1
		//      | b7 << 0
		char c = '\0';
		for (size_t j = 0; j < CODE_LENGTH; j++)
		{
			const size_t shift = CODE_LENGTH - 1 - j;
			if (data[j] > 0)
			{
				c |= (1 << shift);
			}
		}

#ifdef CDMA_DEBUG
		printf("[DEBUG] Receiver: Binary decoded to character = '%c'\n", c);
#endif

		message[i] = c;
	}

#ifdef CDMA_DEBUG
	printf("[DEBUG] Receiver: Decoded message = '%s'\n", message);
#endif

	return message;

l_fail:
	return NULL;
}

char** cdma_receiver_decode_n(
	const cdma_receiver_t* receiver,
	const cdma_encoded_message_t* encoded_message,
	size_t n,
	cdma_errno_t* error_code)
{
	char** messages = NULL;

	if (receiver == NULL || encoded_message == NULL)
	{
		if (error_code != NULL)
		{
			*error_code = CDMA_INVALID_ARGUMENT;
		}
		goto l_fail;
	}

	messages = calloc(n + 1, sizeof(char*));

	if (messages == NULL)
	{
		if (error_code != NULL)
		{
			*error_code = CDMA_OUT_OF_MEMORY;
		}
		goto l_fail;
	}

	for (unsigned int station = 0; station < n; station++)
	{
		char* message = cdma_receiver_get(receiver, encoded_message, station, error_code);

		if (message == NULL)
		{
			free(messages);
			goto l_fail;
		}

		messages[station] = message;
	}

	return messages;

l_fail:
	return NULL;
}

void cdma_receiver_destroy(cdma_receiver_t* receiver)
{
	free(receiver);
}

const char* cdma_strerror(cdma_errno_t error_code)
{
	static const char* strs[] = { [CDMA_SUCCESS] = "", [CDMA_OUT_OF_MEMORY] = "out of memory", [CDMA_INVALID_ARGUMENT] = "invalid argument", [CDMA_BAD_MESSAGE_SIZE] = "bad message size" };
	return strs[error_code];
}

void cdma_encoded_message_destroy(cdma_encoded_message_t* encoded_message)
{
	if (encoded_message != NULL)
	{
		free(encoded_message->encoded);
	}
	free(encoded_message);
}

#include "test-driver.h"

#include "cdma.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void f(const char *s, cdma_errno_t error_code)
{
	fprintf(stderr, "[FATAL] Failed on %s: %s\n", s, cdma_strerror(error_code));
}

static void t(size_t i, const char *actual, const char *expected)
{
	fprintf(stderr, "[ASSERT] From #%zu station expected '%s', but found '%s'\n", i, expected, actual);
}

#define FATAL(condition, s)                                                    \
	if (condition)                                                             \
	{                                                                          \
		f(s, error_code);                                                      \
		exitcode = -1;                                                         \
		goto l_end;                                                            \
	}

#define ASSERT(i, actual, expected)                                            \
	if (strcmp(actual, expected) != 0)                                         \
	{                                                                          \
		t(i, actual, expected);                                                \
		exitcode = -1;                                                         \
		goto l_end;                                                            \
	}

int main(void)
{
	/* Test: number of stations. */
	const size_t n = test_get_n();
	printf("[INFO] Number of stations = %zu\n", n);

	/* Test: messages. */
	const char **messages = test_get_messages();
	for (size_t i = 0; i < n; i++)
	{
		const char *message = messages[i];
		printf("[INFO] Message at #%zu station = '%s'\n", i, message);
	}

	/* Error code for CDMA. */
	cdma_errno_t error_code = CDMA_SUCCESS;

	/* Driver exitcode. */
	int exitcode = 0;

	/* Base station. */
	cdma_base_station_t *base_station = NULL;

	/* Transmitter. */
	cdma_transmitter_t *transmitter = NULL;

	/* Recevier. */
	cdma_receiver_t *receiver = NULL;

	/* Encoded message. */
	cdma_encoded_message_t *encoded_message = NULL;

	/* Decoded messages. */
	char **decode_messages = NULL;

	/* Init base station. */
	base_station = cdma_base_station_create(8, &error_code);
	FATAL(base_station == NULL, "base station init");

	/* Init transmitter. */
	transmitter = cdma_transmitter_create(base_station, &error_code);
	FATAL(base_station == NULL, "transmitter init");

	/* Init recevier. */
	receiver = cdma_receiver_create(base_station, &error_code);
	FATAL(base_station == NULL, "receiver init");

	/* Create stations. */
	for (size_t i = 0; i < n; i++)
	{
		const char *message = messages[i];
		error_code = cdma_transmitter_add_station(transmitter, message);
		FATAL(error_code != CDMA_SUCCESS, "station init");
	}

	/* Create encoded message. */
	encoded_message = cdma_transmitter_send(transmitter, &error_code);
	FATAL(encoded_message == NULL, "encode message");

	/* Decode n messages. */
	decode_messages = cdma_receiver_decode_n(receiver, encoded_message, n, &error_code);
	FATAL(decode_messages == NULL, "decode n messages");

	/* Assert. */
	for (size_t i = 0; i < n; i++)
	{
		const char *actual = decode_messages[i];
		const char *expected = messages[i];
		ASSERT(i, actual, expected);
		printf("[INFO] Decoded message at #%zu station = '%s'\n", i, actual);
	}

	/* Free resources. */
l_end:
	/* Free decode messages. */
	if (decode_messages != NULL)
	{
		for (size_t i = 0; i < n; i++)
		{
			free(decode_messages[i]);
		}
		free(decode_messages);
	}

	/* Free encoded message. */
	cdma_encoded_message_destroy(encoded_message);

	/* Free receiver. */
	cdma_receiver_destroy(receiver);

	/* Free transmitter. */
	cdma_transmitter_destroy(transmitter);

	/* Free base station. */
	cdma_base_station_destroy(base_station);

	return exitcode;
}

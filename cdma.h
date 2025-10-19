#ifndef CDMA_H
#define CDMA_H

#include <stddef.h>
#include <stdint.h>

typedef enum cdma_errno
{
	CDMA_SUCCESS,
	CDMA_OUT_OF_MEMORY,
	CDMA_INVALID_ARGUMENT,
	CDMA_BAD_MESSAGE_SIZE
} cdma_errno_t;

typedef struct cdma_base_station cdma_base_station_t;
typedef struct cdma_transmitter cdma_transmitter_t;
typedef struct cdma_receiver cdma_receiver_t;
typedef struct cdma_encoded_message cdma_encoded_message_t;

/* Creates base station. */
cdma_base_station_t *cdma_base_station_create(unsigned int n, cdma_errno_t *error_code);
/* Destructor for base station. */
void cdma_base_station_destroy(cdma_base_station_t *base_station);

/* Creates transmitter. */
cdma_transmitter_t *cdma_transmitter_create(cdma_base_station_t *base_station, cdma_errno_t *error_code);
/* Adds a new station that broadcasts a message. */
cdma_errno_t cdma_transmitter_add_station(cdma_transmitter_t *transmitter, const char *message);
/* Creates a encoded message. */
cdma_encoded_message_t *cdma_transmitter_send(const cdma_transmitter_t *transmitter, cdma_errno_t *error_code);
/* Destructor for transmitter. */
void cdma_transmitter_destroy(cdma_transmitter_t *transmitter);

/* Creates receiver. */
cdma_receiver_t *cdma_receiver_create(cdma_base_station_t *base_station, cdma_errno_t *error_code);
/* Decodes a single message on station. */
char *cdma_receiver_get(const cdma_receiver_t *receiver, const cdma_encoded_message_t *encoded_message, unsigned int station, cdma_errno_t *error_code);
/* Decodes N messages. */
char **cdma_receiver_decode_n(const cdma_receiver_t *receiver, const cdma_encoded_message_t *encoded_message, unsigned int n, cdma_errno_t *error_code);
/* Destructor for receiver. */
void cdma_receiver_destroy(cdma_receiver_t *receiver);

/* Textual description of error. */
const char *cdma_strerror(cdma_errno_t error_code);
/* Destructor for encoded message. */
void cdma_encoded_message_destroy(cdma_encoded_message_t *encoded_message);

#endif /* CDMA_H */

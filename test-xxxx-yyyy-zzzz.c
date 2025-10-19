#include "test-driver.h"

#include <stddef.h>

size_t test_get_n()
{
	return 3;
}

const char **test_get_messages()
{
	static const char *messages[] = { "xxxx", "yyyy", "zzzz" };
	return messages;
}

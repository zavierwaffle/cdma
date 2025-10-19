#include "test-driver.h"

#include <stddef.h>

size_t test_get_n()
{
	return 4;
}

const char **test_get_messages()
{
	static const char *messages[] = { "GOD", "CAT", "HAM", "SUN" };
	return messages;
}

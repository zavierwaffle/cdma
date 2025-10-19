#include "test-driver.h"

#include <stddef.h>

size_t test_get_n()
{
	return 8;
}

const char **test_get_messages()
{
	static const char *messages[] = {
		"poignant", "affinity", "altruism", "ambience",
		"aflutter", "pleasure", "pottered", "absterge"
	};
	return messages;
}

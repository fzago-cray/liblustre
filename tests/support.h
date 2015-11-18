/* Support functions for the copytool. */

#include <stdbool.h>

int llapi_parse_size(const char *string, unsigned long long *size,
		     unsigned long long *size_units, bool b_is_bytes);
ssize_t strscpy(char *dst, const char *src, size_t dst_size);
ssize_t strscat(char *dst, const char *src, size_t dst_size);

/*
 * Unit testing
 */
void unittest_llapi_parse_size(void);
void unittest_strscpy(void);
void unittest_strscat(void);

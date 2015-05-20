/* Support functions.
 * These should be rolled in the library but the copytool needs them,
 * So put them here for a while.
 */

int llapi_parse_size(const char *string, unsigned long long *size,
		     unsigned long long *size_units, bool b_is_bytes);
ssize_t strscpy(char *dst, const char *src, size_t dst_size);
ssize_t strscat(char *dst, const char *src, size_t dst_size);


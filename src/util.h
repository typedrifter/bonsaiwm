/* See LICENSE.dwm file for copyright and license details. */

void die(const char *fmt, ...);
void *ecalloc(size_t nmemb, size_t size);
int fd_set_nonblock(int fd);

/* parse "#RRGGBB" or "#RRGGBBAA" into normalized RGBA floats.
   returns 0 on success, -1 on malformed input. */
int hex_to_rgba(const char *s, float out[4]);

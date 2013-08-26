#include "gmap.h"

char *UTF8[UTF8_LAST];

void
utf8_init() {
	char tmp[100];
	int len;

	len = g_unichar_to_utf8(0x00b0, tmp);
	tmp[len] = '\0';
	UTF8[UTF8_DEGREES] = gmap_strdup(tmp);
	len = g_unichar_to_utf8(0x2032, tmp);
	tmp[len] = '\0';
	UTF8[UTF8_MINUTES] = gmap_strdup(tmp);
	len = g_unichar_to_utf8(0x2033, tmp);
	tmp[len] = '\0';
	UTF8[UTF8_SECONDS] = gmap_strdup(tmp);
	len = g_unichar_to_utf8 (0x200e, tmp);
	tmp[len] = '\0';
	UTF8[UTF8_LTRMARK] = gmap_strdup(tmp);
}

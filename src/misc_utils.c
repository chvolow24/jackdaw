#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

char *try_fix_api_route(char *route, int route_len) {
    const char *match_strs[] = {
	"equalizer",
	"fir_filter",
	"delay",
	"saturation",
	"compressor"
    };
    int match_strlens[5];
    for (int i=0; i<5; i++) {
	match_strlens[i] = strlen(match_strs[i]);
    }
    int match_str_indices[5] = {0};
    int match_str_starts[5] = {0};
    int match_id = -1;
    for (int i=0; i<route_len; i++) {
	/* fprintf(stderr, "Test char %c\n", route[i]); */
	bool escape_outer = false;
	for (int j=0; j<5; j++) {
	    if (match_strs[j][match_str_indices[j]] == route[i]) {
		/* fprintf(stderr, "\tmatches test str %s (index %d)\n", match_strs[j], match_str_indices[j]); */
		if (match_str_starts[j] == 0) {
		    match_str_starts[j] = i;
		    /* fprintf(stderr, "\t==>set start! %d\n", i); */
		}
		match_str_indices[j]++;
		if (match_str_indices[j] == match_strlens[j]) {
		    match_id = j;
		    escape_outer = true;
		    break;
		}
	    } else {
		match_str_starts[j] = 0;
		match_str_indices[j] = 0;
		/* fprintf(stderr, "\t(no match on str %s, setting start 0)\n", match_strs[j]); */
	    }
	}
	if (escape_outer) break;
    }
    char rest_of_route[route_len];
    int offset = snprintf(rest_of_route, route_len, "%s", route + match_str_starts[match_id] + match_strlens[match_id]);
    rest_of_route[offset] = '\0';
    if (match_id >= 0) {
	int new_route_buf_len = route_len + strlen("/effects");
	char *new_route_buf = malloc(new_route_buf_len + 1);
	route[match_str_starts[match_id]] = '\0';
	snprintf(new_route_buf, new_route_buf_len + 1, "%seffects/%s%s", route, match_strs[match_id], rest_of_route);
	return new_route_buf;
    }
    return NULL;
}

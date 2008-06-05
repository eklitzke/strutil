/* Just do things in C for now without worrying about python api details */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Assumes a byte is 8 bits */
#define SHIFT_WIDTH ((sizeof(int) - sizeof(char)) * 8)

typedef struct {
	int length; /* how many things are in the array */
	char *matches; /* array of characters to match on */
	char **replacements; /* array of strings to replace with */
	int *rlens; /* the lengths of the replacements */
} tr_arr;

char* tr(char *s, tr_arr *rules) {
	int alloc_extra = 1; /* For the terminating null character */
	int i, j;

	const int len = rules->length;

	int s_len = strlen(s);

	/* rbuf is an buffer we use to optimize the replacement strategy. The way
	 * it works is we allocate a buffer of ints on the stack. The high-order
	 * byte of the int is the rule number (because there are at most 256 rules,
	 * one for each ASCII character) and the low order bytes correspond to the
	 * position of the character in the input string. This means that the input
	 * string at most (1 << SHIFT_WIDTH) bytes long (16777216 bytes on a 32-bit
	 * system, 18014398509481984 bytes on a 64-bit system)
	 */

	/* TODO: it might be sensible to allocate rbuf as small (e.g. of length
	 * 100) by default and re-allocate if necessary, since s_len is the most
	 * pessimistic case. OTOH allocating space on the stack is cheap. */

	int rbuf[s_len];
	int r = 0;

	for (i = 0; i < s_len; i++) {
		for (j = 0; j < len; j++) {
			if (s[i] == rules->matches[j]) {
				rbuf[r++] = i | (j << SHIFT_WIDTH);
				alloc_extra += rules->rlens[j] - 1;
				break;
			}
		}
	}

	/* Shortcut case */
	if (r == 0) {
		return strdup(s);
	}

	/* Allocate space for the string we plan on returning */
	char *ns = malloc(s_len + alloc_extra);
	ns[s_len + alloc_extra] = '\0';

	int opos = 0;
	int npos = 0;

	int ps_pos = 0;
	int ns_pos = 0;

	int old_s_pos = 0;

	for (i = 0; i < r; i++) {
		int s_pos = rbuf[i] & ((1 << SHIFT_WIDTH) - 1);
		int r_pos = rbuf[i] >> SHIFT_WIDTH;

		/* all this part is broken */

		int s_copy_chars = s_pos - ps_pos;
		int r_copy_chars = rules->rlens[r_pos];

		/* Copy the next chunk out of s */
		memcpy(ns + ns_pos, s + ps_pos, s_copy_chars);

		/* Copy the replacement chunk */
		memcpy(ns + ns_pos + s_copy_chars, rules->replacements[r_pos], r_copy_chars);

		/* Update the counters */
		ns_pos += (s_copy_chars + r_copy_chars);
		ps_pos = s_pos + 1;

	}

	memcpy(ns + ns_pos, s + ps_pos, s_len - ps_pos);

	return ns;
}

int main(int argc, char **argv) {
	char *input_string = "This string & needs <> to be \"escapes\"& foo";
	printf("input_string = %s\n\n", input_string);

	tr_arr a = { 4, "<>&\"", malloc(4 * sizeof(char *)), malloc(4 * sizeof(int)) };

	a.replacements[0] = strdup("&lt;");
	a.replacements[1] = strdup("&gt;");
	a.replacements[2] = strdup("&amp;");
	a.replacements[3] = strdup("&quot;");

	int i;
	for (i = 0; i < a.length; i++) {
		a.rlens[i] = strlen(a.replacements[i]);
	}

	printf("the first replacement is %s\n", a.replacements[0]);
	printf("the second replacement is %s\n", a.replacements[1]);
	printf("the third replacement is %s\n", a.replacements[2]);
	printf("the fourth replacement is %s\n", a.replacements[3]);

	printf("calling tr...\n\n");
	char *output_string = tr(input_string, &a);
	printf("output_string = %s\n", output_string);
	return 0;
}

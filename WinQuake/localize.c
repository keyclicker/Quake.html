// localize.c -- `$key` strings of the 2021 re-release QuakeC
//
// File format, one entry per line:
//   // comment
//   qc_got_item = "You got {0}\n"   // trailing comment

#include "quakedef.h"
#include "localize.h"

#define LOC_FILE "localization/loc_english.txt"

typedef struct
{
	char *key;
	char *value;
} locentry_t;

static locentry_t *loc_entries;
static int loc_numentries;


static int LOC_Compare (const void *a, const void *b)
{
	return strcmp (((locentry_t *)a)->key, ((locentry_t *)b)->key);
}


/** Unescape the quoted value at `*p` in place; return it, or NULL. */
static char *LOC_ParseValue (char **p)
{
	char *in;
	char *out;
	char *value;

	in = *p;
	if (*in != '"')
		return NULL;

	value = out = ++in;
	for (; *in && *in != '"' && *in != '\n'; in++, out++)
	{
		if (*in == '\\' && in[1])
		{
			in++;
			*out = *in == 'n' ? '\n' : *in;
		}
		else
			*out = *in;
	}

	if (*in != '"')
		return NULL;

	*p = in + 1;
	*out = 0;

	return value;
}


void LOC_Init (void)
{
	char *data;
	char *p;
	char *key;
	char *value;
	int capacity;

	data = (char *)COM_LoadHunkFile (LOC_FILE);
	if (!data)
		return;

	// Every entry has an '=', so that bounds the table
	capacity = 0;
	for (p = data; *p; p++)
		capacity += *p == '=';

	loc_entries = Hunk_AllocName (capacity * sizeof (*loc_entries), "loc");
	loc_numentries = 0;

	for (p = data; *p; )
	{
		// Line start: skip blanks, then read `key`
		while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
			p++;
		key = p;
		while (*p && *p != ' ' && *p != '\t' && *p != '=' && *p != '\n')
			p++;

		if (*p && *p != '\n' && p > key && key[0] != '/')
		{
			*p++ = 0;
			while (*p == ' ' || *p == '\t' || *p == '=')
				p++;

			value = LOC_ParseValue (&p);
			if (value)
			{
				loc_entries[loc_numentries].key = key;
				loc_entries[loc_numentries].value = value;
				loc_numentries++;
			}
		}

		// Drop the rest of the line, comments included
		while (*p && *p != '\n')
			p++;
	}

	qsort (loc_entries, loc_numentries, sizeof (*loc_entries), LOC_Compare);
	Con_Printf ("%s: %i strings\n", LOC_FILE, loc_numentries);
}


char *LOC_Get (char *s)
{
	locentry_t needle;
	locentry_t *found;

	if (s[0] != '$' || !loc_numentries)
		return s;

	needle.key = s + 1;
	found = bsearch (&needle, loc_entries, loc_numentries,
		sizeof (*loc_entries), LOC_Compare);

	return found ? found->value : s;
}


void LOC_Format (char *fmt, char **args, int numargs, char *out,
	int outsize)
{
	char *in;
	char *arg;
	int next;
	int index;
	int len;

	next = 0;
	len = 0;
	outsize--;  // keep room for the terminator

	for (in = LOC_Get (fmt); *in && len < outsize; in++)
	{
		// `{}` or `{N}`: splice in an argument
		if (in[0] == '{' && (in[1] == '}' ||
			(in[1] >= '0' && in[1] <= '9' && in[2] == '}')))
		{
			index = in[1] == '}' ? next++ : in[1] - '0';
			in += in[1] == '}' ? 1 : 2;

			if (index >= numargs)
				continue;

			for (arg = LOC_Get (args[index]); *arg && len < outsize; arg++)
				out[len++] = *arg;

			continue;
		}

		out[len++] = *in;
	}

	out[len] = 0;
}

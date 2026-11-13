// localize.h -- `$key` strings of the 2021 re-release QuakeC

/** Load `localization/loc_english.txt`, if the game has one. */
void LOC_Init (void);

/** Return the text for `$key`, or `s` itself if it isn't one we know. */
char *LOC_Get (char *s);

/**
 * Expand a localized format into `out`.
 *
 * `fmt` goes through `LOC_Get`, then `{0}`..`{9}` take that `args` entry
 * and each bare `{}` takes the next one. Args are localized too.
 */
void LOC_Format (char *fmt, char **args, int numargs, char *out,
	int outsize);

// sys_web.c -- browser system driver: main loop, files, time, errors

#include <errno.h>
#include <sys/stat.h>

#include <emscripten.h>

#include "quakedef.h"

#define DEFAULT_MEMSIZE (64 * 1024 * 1024)

// Game data lives here in the Emscripten FS; `web/shell/quake.html`
// fetches it before `main` runs.
#define BASEDIR "/quake"

qboolean isDedicated;  // `host.c` asks; a browser tab is never a server

/*
===============================================================================

FILE IO

===============================================================================
*/

#define MAX_HANDLES 32

static FILE *sys_handles[MAX_HANDLES];


/** Return a free slot in `sys_handles`, or die. */
static int findhandle (void)
{
	int i;

	for (i = 1; i < MAX_HANDLES; i++)
		if (!sys_handles[i])
			return i;

	Sys_Error ("out of handles");
	return -1;
}


static int filelength (FILE *f)
{
	int pos;
	int end;

	pos = ftell (f);
	fseek (f, 0, SEEK_END);
	end = ftell (f);
	fseek (f, pos, SEEK_SET);

	return end;
}


int Sys_FileOpenRead (char *path, int *hndl)
{
	FILE *f;
	int i;

	i = findhandle ();
	f = fopen (path, "rb");

	if (!f)
	{
		*hndl = -1;
		return -1;
	}

	sys_handles[i] = f;
	*hndl = i;

	return filelength (f);
}


int Sys_FileOpenWrite (char *path)
{
	FILE *f;
	int i;

	i = findhandle ();
	f = fopen (path, "wb");

	if (!f)
		Sys_Error ("Error opening %s: %s", path, strerror (errno));

	sys_handles[i] = f;

	return i;
}


void Sys_FileClose (int handle)
{
	fclose (sys_handles[handle]);
	sys_handles[handle] = NULL;
}


void Sys_FileSeek (int handle, int position)
{
	fseek (sys_handles[handle], position, SEEK_SET);
}


int Sys_FileRead (int handle, void *dest, int count)
{
	return fread (dest, 1, count, sys_handles[handle]);
}


int Sys_FileWrite (int handle, void *data, int count)
{
	return fwrite (data, 1, count, sys_handles[handle]);
}


int Sys_FileTime (char *path)
{
	struct stat buf;

	if (stat (path, &buf) == -1)
		return -1;

	return buf.st_mtime;
}


void Sys_mkdir (char *path)
{
	mkdir (path, 0777);
}

/*
===============================================================================

SYSTEM IO

===============================================================================
*/

void Sys_MakeCodeWriteable (unsigned long startaddr, unsigned long length)
{
}


void Sys_DebugLog (char *file, char *fmt, ...)
{
}


// Show the message in the page, then stop the main loop for good.
EM_JS (void, js_fatal, (const char *message), {
	Module.quakeFatal(UTF8ToString(message));
});


void Sys_Error (char *error, ...)
{
	va_list argptr;
	char text[1024];

	va_start (argptr, error);
	vsnprintf (text, sizeof (text), error, argptr);
	va_end (argptr);

	printf ("Sys_Error: %s\n", text);

	emscripten_cancel_main_loop ();
	js_fatal (text);
	emscripten_force_exit (1);
}


void Sys_Printf (char *fmt, ...)
{
	va_list argptr;
	char text[2048];
	char *c;

	va_start (argptr, fmt);
	vsnprintf (text, sizeof (text), fmt, argptr);
	va_end (argptr);

	// Strip Quake's "bronze" high bit so the browser console reads ASCII
	for (c = text; *c; c++)
		*c &= 0x7f;

	fputs (text, stdout);
}


EM_JS (void, js_quit, (void), {
	Module.quakeQuit();
});


void Sys_Quit (void)
{
	Host_Shutdown ();

	emscripten_cancel_main_loop ();
	js_quit ();
	emscripten_force_exit (0);
}


double Sys_FloatTime (void)
{
	return emscripten_get_now () / 1000.0;
}


char *Sys_ConsoleInput (void)
{
	return NULL;
}


void Sys_Sleep (void)
{
}


// Browser events arrive through html5.h callbacks between frames.
void Sys_SendKeyEvents (void)
{
}


void Sys_HighFPPrecision (void)
{
}


void Sys_LowFPPrecision (void)
{
}

/*
===============================================================================

MAIN LOOP

===============================================================================
*/

static double oldtime;


/** One browser animation frame: one `Host_Frame`. */
static void frame (void)
{
	double newtime;

	newtime = Sys_FloatTime ();
	Host_Frame (newtime - oldtime);
	oldtime = newtime;
}


int main (int argc, char **argv)
{
	static quakeparms_t parms;
	int i;

	COM_InitArgv (argc, argv);

	parms.memsize = DEFAULT_MEMSIZE;
	i = COM_CheckParm ("-mem");
	if (i && i + 1 < com_argc)
		parms.memsize = Q_atoi (com_argv[i + 1]) * 1024 * 1024;

	parms.membase = malloc (parms.memsize);
	parms.basedir = BASEDIR;
	parms.argc = com_argc;
	parms.argv = com_argv;

	if (!parms.membase)
		Sys_Error ("Can't allocate %d bytes", parms.memsize);

	Host_Init (&parms);

	// Never returns: the browser calls `frame` on each animation frame
	oldtime = Sys_FloatTime ();
	emscripten_set_main_loop (frame, 0, 1);

	return 0;
}

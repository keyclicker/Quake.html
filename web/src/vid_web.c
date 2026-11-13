// vid_web.c -- browser video driver: 8-bit framebuffer -> RGBA -> <canvas>

#include <emscripten.h>

#include "quakedef.h"
#include "d_local.h"

#define DEFAULT_WIDTH  640
#define DEFAULT_HEIGHT 480

viddef_t vid;

unsigned short d_8to16table[256];
unsigned d_8to24table[256];

static byte *vid_buffer;
static short *zbuffer;
static byte *surfcache;

// RGBA pixels as the canvas wants them, and the palette that fills them
static unsigned *rgba_buffer;
static unsigned rgba_palette[256];


EM_JS (void, js_vid_init, (int width, int height), {
	const canvas = Module.canvas;

	canvas.width = width;
	canvas.height = height;
	Module.quakeContext = canvas.getContext("2d", { alpha: false });
});


// A fresh view per frame: memory growth detaches old views of HEAPU8.
EM_JS (void, js_vid_present, (const void *pixels, int width, int height), {
	const bytes = new Uint8ClampedArray(HEAPU8.buffer, pixels,
		width * height * 4);

	Module.quakeContext.putImageData(new ImageData(bytes, width, height),
		0, 0);
});


/** Read `-name <int>` from the command line, or return `fallback`. */
static int vid_arg (char *name, int fallback, int max)
{
	int i;
	int value;

	i = COM_CheckParm (name);
	if (!i || i + 1 >= com_argc)
		return fallback;

	value = Q_atoi (com_argv[i + 1]);
	if (value < 320 || value > max)
		Sys_Error ("%s must be in 320..%d", name, max);

	return value;
}


void VID_SetPalette (unsigned char *palette)
{
	int i;
	byte *p;

	// Little-endian RGBA: R in the low byte, opaque alpha on top
	for (i = 0, p = palette; i < 256; i++, p += 3)
		rgba_palette[i] = 0xff000000 | (p[2] << 16) | (p[1] << 8) | p[0];
}


void VID_ShiftPalette (unsigned char *palette)
{
	VID_SetPalette (palette);
}


void VID_Init (unsigned char *palette)
{
	int width;
	int height;
	int cachesize;

	width = vid_arg ("-width", DEFAULT_WIDTH, MAXWIDTH);
	height = vid_arg ("-height", DEFAULT_HEIGHT, MAXHEIGHT);

	vid.maxwarpwidth = WARP_WIDTH;
	vid.maxwarpheight = WARP_HEIGHT;
	vid.width = vid.conwidth = width;
	vid.height = vid.conheight = height;
	vid.rowbytes = vid.conrowbytes = width;
	vid.aspect = ((float)height / (float)width) * (320.0 / 240.0);
	vid.numpages = 1;
	vid.colormap = host_colormap;
	vid.fullbright = 256 - LittleLong (*((int *)vid.colormap + 2048));

	// Buffers sized for this mode, allocated once
	cachesize = D_SurfaceCacheForRes (width, height);
	vid_buffer = malloc (width * height);
	zbuffer = malloc (width * height * sizeof (*zbuffer));
	surfcache = malloc (cachesize);
	rgba_buffer = malloc (width * height * sizeof (*rgba_buffer));

	if (!vid_buffer || !zbuffer || !surfcache || !rgba_buffer)
		Sys_Error ("VID_Init: out of memory for %dx%d", width, height);

	vid.buffer = vid.conbuffer = vid_buffer;
	d_pzbuffer = zbuffer;
	D_InitCaches (surfcache, cachesize);

	VID_SetPalette (palette);
	js_vid_init (width, height);
}


void VID_Shutdown (void)
{
}


// Quake marks dirty rects, but converting the whole frame is cheap enough.
void VID_Update (vrect_t *rects)
{
	int i;
	int count;

	count = vid.width * vid.height;
	for (i = 0; i < count; i++)
		rgba_buffer[i] = rgba_palette[vid_buffer[i]];

	js_vid_present (rgba_buffer, vid.width, vid.height);
}


// Disk-loading icon: the browser loads from memory, nothing to show.
void D_BeginDirectRect (int x, int y, byte *pbitmap, int width, int height)
{
}


void D_EndDirectRect (int x, int y, int width, int height)
{
}

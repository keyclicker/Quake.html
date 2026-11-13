// in_web.c -- browser input driver: keyboard, pointer-locked mouse

#include <string.h>

#include <emscripten/html5.h>

#include "quakedef.h"

#define CANVAS "#canvas"

static cvar_t m_filter = {"m_filter", "0"};

// Mouse motion summed between frames, consumed by `IN_Move`
static int mouse_dx;
static int mouse_dy;
static int old_mouse_dx;
static int old_mouse_dy;
static qboolean mouse_locked;

/*
===============================================================================

KEYBOARD

===============================================================================
*/

typedef struct
{
	const char *code;   // `KeyboardEvent.code`: physical, layout-independent
	int key;            // Quake `K_*` or unshifted ASCII
} keymap_t;

static const keymap_t keymap[] =
{
	{"Tab", K_TAB},             {"Enter", K_ENTER},
	{"NumpadEnter", K_ENTER},   {"Escape", K_ESCAPE},
	{"Space", K_SPACE},         {"Backspace", K_BACKSPACE},

	{"ArrowUp", K_UPARROW},     {"ArrowDown", K_DOWNARROW},
	{"ArrowLeft", K_LEFTARROW}, {"ArrowRight", K_RIGHTARROW},

	{"AltLeft", K_ALT},         {"AltRight", K_ALT},
	{"ControlLeft", K_CTRL},    {"ControlRight", K_CTRL},
	{"ShiftLeft", K_SHIFT},     {"ShiftRight", K_SHIFT},

	{"F1", K_F1},   {"F2", K_F2},   {"F3", K_F3},   {"F4", K_F4},
	{"F5", K_F5},   {"F6", K_F6},   {"F7", K_F7},   {"F8", K_F8},
	{"F9", K_F9},   {"F10", K_F10}, {"F11", K_F11}, {"F12", K_F12},

	{"Insert", K_INS},          {"Delete", K_DEL},
	{"PageDown", K_PGDN},       {"PageUp", K_PGUP},
	{"Home", K_HOME},           {"End", K_END},
	{"Pause", K_PAUSE},

	{"Minus", '-'},             {"Equal", '='},
	{"BracketLeft", '['},       {"BracketRight", ']'},
	{"Backslash", '\\'},        {"Semicolon", ';'},
	{"Quote", '\''},            {"Backquote", '`'},
	{"Comma", ','},             {"Period", '.'},
	{"Slash", '/'},
};


/** Map a DOM key code to a Quake key, or 0 if Quake has no such key. */
static int map_key (const char *code)
{
	int i;

	if (!strncmp (code, "Key", 3) && code[3] && !code[4])
		return code[3] - 'A' + 'a';

	if (!strncmp (code, "Digit", 5) && code[5] && !code[6])
		return code[5];

	for (i = 0; i < sizeof (keymap) / sizeof (keymap[0]); i++)
		if (!strcmp (code, keymap[i].code))
			return keymap[i].key;

	return 0;
}


static EM_BOOL on_key (int type, const EmscriptenKeyboardEvent *e,
	void *user)
{
	int key;

	key = map_key (e->code);
	if (!key)
		return EM_FALSE;

	Key_Event (key, type == EMSCRIPTEN_EVENT_KEYDOWN);

	// Leave fullscreen and devtools to the browser
	return key != K_F11 && key != K_F12;
}


static EM_BOOL on_blur (int type, const EmscriptenFocusEvent *e, void *user)
{
	Key_ClearStates ();
	return EM_FALSE;
}

/*
===============================================================================

MOUSE

===============================================================================
*/

/** DOM `MouseEvent.button` (0 left, 1 middle, 2 right) -> `K_MOUSE*`. */
static int map_button (unsigned short button)
{
	switch (button)
	{
	case 0: return K_MOUSE1;
	case 1: return K_MOUSE3;
	case 2: return K_MOUSE2;
	default: return 0;
	}
}


static EM_BOOL on_mouse (int type, const EmscriptenMouseEvent *e,
	void *user)
{
	int key;

	// First click only grabs the mouse; it doesn't fire
	if (!mouse_locked)
	{
		if (type == EMSCRIPTEN_EVENT_MOUSEDOWN)
			emscripten_request_pointerlock (CANVAS, EM_TRUE);
		return EM_TRUE;
	}

	switch (type)
	{
	case EMSCRIPTEN_EVENT_MOUSEMOVE:
		mouse_dx += e->movementX;
		mouse_dy += e->movementY;
		return EM_TRUE;

	case EMSCRIPTEN_EVENT_MOUSEDOWN:
	case EMSCRIPTEN_EVENT_MOUSEUP:
		key = map_button (e->button);
		if (key)
			Key_Event (key, type == EMSCRIPTEN_EVENT_MOUSEDOWN);
		return EM_TRUE;
	}

	return EM_FALSE;
}


static EM_BOOL on_wheel (int type, const EmscriptenWheelEvent *e,
	void *user)
{
	int key;

	if (e->deltaY == 0)
		return EM_FALSE;

	key = e->deltaY < 0 ? K_MWHEELUP : K_MWHEELDOWN;
	Key_Event (key, true);
	Key_Event (key, false);

	return EM_TRUE;
}


static EM_BOOL on_pointerlock (int type,
	const EmscriptenPointerlockChangeEvent *e, void *user)
{
	mouse_locked = e->isActive;

	// Buttons held while the lock broke never get their mouseup
	if (!mouse_locked)
	{
		Key_Event (K_MOUSE1, false);
		Key_Event (K_MOUSE2, false);
		Key_Event (K_MOUSE3, false);
		mouse_dx = mouse_dy = 0;
	}

	return EM_FALSE;
}

/*
===============================================================================

DRIVER INTERFACE

===============================================================================
*/

void IN_Init (void)
{
	const char *win = EMSCRIPTEN_EVENT_TARGET_WINDOW;
	const char *doc = EMSCRIPTEN_EVENT_TARGET_DOCUMENT;

	Cvar_RegisterVariable (&m_filter);

	emscripten_set_keydown_callback (win, NULL, EM_TRUE, on_key);
	emscripten_set_keyup_callback (win, NULL, EM_TRUE, on_key);
	emscripten_set_blur_callback (win, NULL, EM_TRUE, on_blur);

	emscripten_set_mousedown_callback (CANVAS, NULL, EM_TRUE, on_mouse);
	emscripten_set_mouseup_callback (doc, NULL, EM_TRUE, on_mouse);
	emscripten_set_mousemove_callback (doc, NULL, EM_TRUE, on_mouse);
	emscripten_set_wheel_callback (CANVAS, NULL, EM_TRUE, on_wheel);
	emscripten_set_pointerlockchange_callback (doc, NULL, EM_TRUE,
		on_pointerlock);
}


void IN_Shutdown (void)
{
	emscripten_html5_remove_all_event_listeners ();
}


void IN_Commands (void)
{
}


/** Turn summed mouse motion into view angles or movement (`in_win.c`). */
void IN_Move (usercmd_t *cmd)
{
	float mouse_x;
	float mouse_y;

	if (m_filter.value)
	{
		mouse_x = (mouse_dx + old_mouse_dx) * 0.5;
		mouse_y = (mouse_dy + old_mouse_dy) * 0.5;
	}
	else
	{
		mouse_x = mouse_dx;
		mouse_y = mouse_dy;
	}

	old_mouse_dx = mouse_dx;
	old_mouse_dy = mouse_dy;
	mouse_dx = mouse_dy = 0;

	mouse_x *= sensitivity.value;
	mouse_y *= sensitivity.value;

	// Horizontal: turn, or strafe
	if ((in_strafe.state & 1) || (lookstrafe.value && (in_mlook.state & 1)))
		cmd->sidemove += m_side.value * mouse_x;
	else
		cl.viewangles[YAW] -= m_yaw.value * mouse_x;

	if (in_mlook.state & 1)
		V_StopPitchDrift ();

	// Vertical: look, or walk
	if ((in_mlook.state & 1) && !(in_strafe.state & 1))
	{
		cl.viewangles[PITCH] += m_pitch.value * mouse_y;
		if (cl.viewangles[PITCH] > 80)
			cl.viewangles[PITCH] = 80;
		if (cl.viewangles[PITCH] < -70)
			cl.viewangles[PITCH] = -70;
	}
	else if ((in_strafe.state & 1) && noclip_anglehack)
		cmd->upmove -= m_forward.value * mouse_y;
	else
		cmd->forwardmove -= m_forward.value * mouse_y;
}

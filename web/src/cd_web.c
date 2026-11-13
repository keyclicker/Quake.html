// cd_web.c -- browser "CD audio": CD track N -> `id1/music/trackNN.ogg`
//
// Tracks stream through an <audio> element straight from their URL
// (`Module.quakeMusicUrl`), so they never enter the Emscripten FS.

#include <emscripten.h>

#include "quakedef.h"

static float old_volume = -1;


EM_JS (void, js_cd_play, (int track, int looping), {
	const music = Module.quakeMusic;
	const url = Module.quakeMusicUrl && Module.quakeMusicUrl(track);

	music.pending = false;
	music.audio.pause();
	if (!url) {
		return;
	}

	music.audio.src = url;
	music.audio.loop = !!looping;
	music.audio.play().catch(() => {
		music.pending = true;
	});
});


EM_JS (void, js_cd_stop, (void), {
	const music = Module.quakeMusic;

	music.pending = false;
	music.audio.pause();
	music.audio.removeAttribute("src");
});


EM_JS (void, js_cd_pause, (int paused), {
	const audio = Module.quakeMusic.audio;

	if (paused) {
		audio.pause();
	} else if (audio.src) {
		audio.play().catch(() => {});
	}
});


EM_JS (void, js_cd_volume, (float volume), {
	Module.quakeMusic.audio.volume = Math.min(Math.max(volume, 0), 1);
});


EM_JS (void, js_cd_init, (void), {
	const music = Module.quakeMusic = {
		audio: new Audio(),
		pending: false,
	};

	// Autoplay rules: a track started before any gesture waits for one
	const retry = () => {
		if (music.pending) {
			music.pending = false;
			music.audio.play().catch(() => {});
		}
	};
	for (const type of ["keydown", "mousedown", "touchstart"]) {
		addEventListener(type, retry, { capture: true });
	}

	music.audio.addEventListener("error", () => {
		console.warn("Quake music: can't play", music.audio.src);
	});
});


void CDAudio_Play (byte track, qboolean looping)
{
	js_cd_play (track, looping);
}


void CDAudio_Stop (void)
{
	js_cd_stop ();
}


void CDAudio_Pause (void)
{
	js_cd_pause (true);
}


void CDAudio_Resume (void)
{
	js_cd_pause (false);
}


// Follow the "CD Music Volume" slider (`bgmvolume`).
void CDAudio_Update (void)
{
	if (bgmvolume.value == old_volume)
		return;

	old_volume = bgmvolume.value;
	js_cd_volume (bgmvolume.value);
}


int CDAudio_Init (void)
{
	js_cd_init ();
	return 0;
}


void CDAudio_Shutdown (void)
{
	CDAudio_Stop ();
}

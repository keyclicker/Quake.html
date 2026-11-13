// snd_web.c -- browser sound driver: Quake's DMA ring -> AudioWorklet
//
// Quake mixes ahead into a ring of `RING_FRAMES` stereo frames and asks
// `SNDDMA_GetDMAPos` where playback is. Here that position is the
// AudioContext's own frame counter, so no clock can drift. Each
// `SNDDMA_Submit` posts the newly mixed frames to a worklet that keeps
// the same ring, indexed by frame: it plays `ring[currentFrame & mask]`
// and zeroes each slot it plays, so a stalled tab plays silence, not
// stale sound.

#include <emscripten.h>

#include "quakedef.h"

#define RING_FRAMES 16384   // power of two; ~0.34 s at 48 kHz
#define CHANNELS    2

static int submitted;       // `paintedtime` sent so far, in frames


/** Create the AudioContext and start loading the worklet; return rate. */
EM_JS (int, js_snd_init, (int ring_frames), {
	if (!globalThis.AudioContext) {
		return 0;
	}

	// `audioWorklet` exists only in secure contexts (https, localhost)
	const ctx = new AudioContext();
	if (!ctx.audioWorklet) {
		ctx.close();
		return 0;
	}

	// Runs inside AudioWorkletGlobalScope, shipped as a Blob: single file
	function worklet() {
		class QuakeOutput extends AudioWorkletProcessor {
			constructor(options) {
				super();
				const frames = options.processorOptions.frames;
				this.mask = frames - 1;
				this.ring = new Float32Array(frames * 2);
				this.port.onmessage = (e) => this.write(e.data);
			}

			write({ start, samples }) {
				for (let i = 0; i < samples.length; i += 2) {
					const slot = ((start + i / 2) & this.mask) * 2;
					this.ring[slot] = samples[i];
					this.ring[slot + 1] = samples[i + 1];
				}
			}

			process(inputs, outputs) {
				const [left, right] = outputs[0];
				for (let i = 0; i < left.length; i++) {
					const slot = ((currentFrame + i) & this.mask) * 2;
					left[i] = this.ring[slot];
					right[i] = this.ring[slot + 1];
					this.ring[slot] = this.ring[slot + 1] = 0;
				}
				return true;
			}
		}

		registerProcessor("quake-output", QuakeOutput);
	}

	const source = "(" + worklet.toString() + ")();";
	const url = URL.createObjectURL(
		new Blob([source], { type: "text/javascript" }));

	const snd = Module.quakeSound = { ctx: ctx, node: null };

	ctx.audioWorklet.addModule(url).then(() => {
		snd.node = new AudioWorkletNode(ctx, "quake-output", {
			outputChannelCount: [2],
			processorOptions: { frames: ring_frames },
		});
		snd.node.connect(ctx.destination);
	}).catch((err) => console.warn("Quake sound disabled:", err));

	// Autoplay rules: the context only starts after a user gesture
	const resume = () => ctx.state === "suspended" && ctx.resume();
	for (const type of ["keydown", "mousedown", "touchstart"]) {
		addEventListener(type, resume, { capture: true });
	}

	return ctx.sampleRate;
});


/** Frame the audio hardware plays next, per the AudioContext clock. */
EM_JS (double, js_snd_frame, (void), {
	const ctx = Module.quakeSound.ctx;
	return Math.floor(ctx.currentTime * ctx.sampleRate);
});


/** Post `count` frames from the int16 ring, starting at frame `start`. */
EM_JS (void, js_snd_submit,
	(const short *ring, int ring_frames, int start, int count), {
	const node = Module.quakeSound.node;
	if (!node) {
		return;
	}

	const mask = ring_frames - 1;
	const pcm = HEAP16.subarray(ring >> 1, (ring >> 1) + ring_frames * 2);
	const samples = new Float32Array(count * 2);

	for (let i = 0; i < count; i++) {
		const slot = ((start + i) & mask) * 2;
		samples[i * 2] = pcm[slot] / 32768;
		samples[i * 2 + 1] = pcm[slot + 1] / 32768;
	}

	node.port.postMessage({ start: start & mask, samples: samples },
		[samples.buffer]);
});


EM_JS (void, js_snd_shutdown, (void), {
	const snd = Module.quakeSound;
	if (snd) {
		snd.ctx.close();
		Module.quakeSound = null;
	}
});


qboolean SNDDMA_Init (void)
{
	int rate;

	rate = js_snd_init (RING_FRAMES);
	if (!rate)
	{
		Con_Printf ("SNDDMA_Init: no AudioWorklet in this browser\n");
		return false;
	}

	// `sn` is the `dma_t` that `snd_dma.c` owns; we fill it in
	memset ((void *)&sn, 0, sizeof (sn));
	sn.splitbuffer = false;
	sn.channels = CHANNELS;
	sn.samplebits = 16;
	sn.speed = rate;
	sn.samples = RING_FRAMES * CHANNELS;
	sn.submission_chunk = 1;
	sn.samplepos = 0;
	sn.buffer = calloc (sn.samples, sn.samplebits / 8);

	if (!sn.buffer)
		return false;

	shm = &sn;
	submitted = 0;

	return true;
}


// In mono samples, wrapped to the ring, as `snd_dma.c` expects.
int SNDDMA_GetDMAPos (void)
{
	int frame;

	frame = (long long)js_snd_frame () & (RING_FRAMES - 1);
	shm->samplepos = frame * CHANNELS;

	return shm->samplepos;
}


void SNDDMA_Submit (void)
{
	int start;
	int count;

	// After a stall, only the last ring's worth of mixing is still valid
	start = submitted;
	if (paintedtime - start > RING_FRAMES)
		start = paintedtime - RING_FRAMES;

	count = paintedtime - start;
	if (count > 0)
		js_snd_submit ((short *)shm->buffer, RING_FRAMES, start, count);

	submitted = paintedtime;
}


void SNDDMA_Shutdown (void)
{
	js_snd_shutdown ();
	free ((void *)sn.buffer);
	sn.buffer = NULL;
	shm = NULL;
}

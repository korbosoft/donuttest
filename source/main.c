#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <aesndlib.h>
#include <gcmodplay.h>
#include <grrlib.h>

#include "colors.h"
#include "donut.h"
#include "grrproxy.h"
#include "strings.h"
#include "text.h"

#include "music_mod.h"

#define DEFAULT_FIFO_SIZE	(256*1024)

// static GXRModeObj *rmode = NULL;
static void *cxfb = NULL;
static MODPlay play;

#define SPLASH_COUNT 6

static const char *splashMessages[SPLASH_COUNT] = {
	[0] = "Also try DS Donut!",
	[1] = "Better than Wii Donut! [citation needed]",
	[2] = "oh man please to help i am not good with c",
	[3] = "(\"Doughnut\" if you're british)",
	[4] = "Korbo loves you <3",
	[5] = "Did you know you can change the music?",
};

int main(int argc,char **argv) {
	char splash[43], title[82], frostingName[82], doughName[82];
	f32 yscale = 0;
	u32 xfbHeight;
	u32 fb = 0;
	u64 frame = 0;
	Mtx view;
	Mtx44 perspective;
	void *gpfifo = NULL;
	guVector cam = {0.0F, 0.0F, 0.0F},
	up = {0.0F, 1.0F, 0.0F},
	look = {0.0F, 0.0F, -1.0F};

	GRRLIB_Init();

	WPAD_Init();
	// WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS_ACC);

	AESND_Init();

	// Allocate memory for the display in the uncached region
	cxfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	VIDEO_ClearFrameBuffer(rmode, cxfb, COLOR_BLACK);

	// Initialise the console, required for printf
	console_init(cxfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
	//SYS_STDIO_Report(true);
	VIDEO_SetNextFramebuffer(cxfb);

	// setup our projection matrix
	float aspect;
	if (CONF_GetAspectRatio() == CONF_ASPECT_16_9) {
		aspect = 16.0f/9.0f;
	} else {
		aspect = 4.0f/3.0f;
	}

	float donAspect = aspect;

	donAspect *= 75.0f / 44.0f; // effectively halves the width to match the character aspect
	GRRLIB_SetLightAmbient(0x303030FF);
	GX_SetChanMatColor(GX_COLOR0A0, LC_WHITE);
	// char boobs[] = " -:=+>|%}Ics1aeCo34wSZkhAE&D$HWQ";

	// gensqrtlut();

	MODPlay_Init(&play);
	MODPlay_SetMOD(&play, music_mod);
	MODPlay_SetVolume(&play,63,63);
	MODPlay_Start(&play);
	PROXY_3dMode(0.1F, 300.0F, 45, false, true, donAspect);
	PROXY_Camera3dSettings(0.0f,0.0f,0.0f, 0,1,0, 0,0,0);

	donut_init();

	float A = 1, B = 1;

	if (rand() % 49) {
		format_splash(splashMessages[rand() % SPLASH_COUNT] ?: "FLAGRANT SPLASH ERROR", splash);
	} else {
		format_splash("This splash has a 1/50 chance of appearing", splash);
	}

	while(SYS_MainLoop()) {
		GXLightObj lobj;
		guVector lpos = {2.0f, 2.0f, 2.0f};
		guVecMultiply(view, &lpos, &lpos);
		GRRLIB_SetLightDiff(0, (guVector){0, 0, 0}, 0.5f, 0.998f, 0xFFFFFFFF);

		draw_donut(A, B);

		WPAD_ScanPads();

		printf("\e[23;0H" "\e[104;37m"
		"╔═══════════════════════════════════════════════════════════════════════════╗"
		"║ \e[4mKorbo's Wii Donut Mod %s   %s\e[0m\e[104;37m "                      "║"
		"║ Based on the original donut.c by Andy Sloane <andy@a1k0n.net>             ║"
		"║ Ported by emilydaemon <emilydaemon@donut.eu.org>, Modified by Korbo       ║"
		"║ Default Music by Jogeir Liljedahl                 " STRING_CONTROLS     " ║"
		"╚═══════════════════════════════════════════════════════════════════════════╝\e[40m", VERSION, splash);

		VIDEO_Flush();
		VIDEO_WaitVSync();
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME) break;

		A += 0.035f;
		B += 0.01f;
	}

	GRRLIB_2dMode();
	donut_exit();
	GRRLIB_Exit();
	MODPlay_Stop(&play);
	exit(0);
}

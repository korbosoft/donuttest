#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <aesndlib.h>
#include <gcmodplay.h>

#include "graphicsUtililiys.h"
#include "primitives.h"
#include "utililiys.h"

#include "Torus_liym3q.h"
#include "shape_lut_bin.h"

#include "music_mod.h"

#define DEFAULT_FIFO_SIZE	(256*1024)

static GXRModeObj *rmode = NULL;
static void *xfb = NULL;
static MODPlay play;

int main(int argc,char **argv) {

	f32 yscale = 0;
	u32 xfbHeight;
	u32 fb = 0;
	u64 frame = 0;
	Mtx view, barview, model, model2;
	Mtx44 donperspective, barperspective;
	void *gpfifo = NULL;
	guVector cam = {0.0F, 0.0F, 0.0F},
	up = {0.0F, 1.0F, 0.0F},
	look = {0.0F, 0.0F, -1.0F};

	VIDEO_Init();

	WPAD_Init();
	WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS_ACC);

	AESND_Init();

	rmode = VIDEO_GetPreferredMode(NULL);

	rmode->viWidth = 710;
	rmode->viXOrigin = 5;

	// allocate the fifo buffer
	gpfifo = memalign(32,DEFAULT_FIFO_SIZE);
	memset(gpfifo,0,DEFAULT_FIFO_SIZE);

	// Allocate memory for the display in the uncached region
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	VIDEO_ClearFrameBuffer(rmode, xfb, COLOR_BLACK);

	// Initialise the console, required for printf
	console_init(xfb,20,20,rmode->fbWidth-20,rmode->xfbHeight-20,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
	//SYS_STDIO_Report(true);
	VIDEO_SetNextFramebuffer(xfb);

	// configure video
	VIDEO_Configure(rmode);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

	GX_Init(gpfifo,DEFAULT_FIFO_SIZE);

	GX_SetCopyClear(LC_BLACK, 0x00ffffff);

	int donwidth = 75;
	int donheight = 22;
	yscale = GX_GetYScaleFactor(480, rmode->xfbHeight);
	xfbHeight = GX_SetDispCopyYScale(yscale);
	GX_SetCopyFilter(GX_FALSE, NULL, GX_FALSE, NULL);
	GX_SetFieldMode(rmode->field_rendering,((rmode->viHeight==2*rmode->xfbHeight)?GX_ENABLE:GX_DISABLE));

	GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);
	GX_SetDispCopyGamma((f32)GX_GM_1_0);

	// setup our projection matrix
	float aspect;
	if (CONF_GetAspectRatio() == CONF_ASPECT_16_9) {
		aspect = 16.0f/9.0f;
	} else {
		aspect = 4.0f/3.0f;
	}

	float donaspect = aspect, baraspect = aspect;

	donaspect *= 28.0f / 22.0f;
	baraspect *= 480.0f / 100.0f;

	guPerspective(donperspective, 45, donaspect, 0.1F, 300.0F);
	guPerspective(barperspective, 45, baraspect, 0.1F, 300.0F);

	guLookAt(barview, &cam, &up, &look);

	//set number of textures to generate   <-- absolutely hilarious comment from libogc example code
	GX_SetNumTexGens(1);
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

	GX_SetNumChans(1);
	GX_SetChanAmbColor(GX_COLOR0A0, LC_DARKISH);
	GX_SetChanMatColor(GX_COLOR0A0, LC_WHITE);

	// char boobs[] = " -:=+>|%}Ics1aeCo34wSZkhAE&D$HWQ";

	gensqrtlut();

	void * bin = LilyCoolMalloc(640*528*2); // i want to clear the efb with the blitter without saving the data so it goes here
	u32 *capture_buffer = (u32*)memalign(32, rmode->fbWidth * rmode->xfbHeight * 4);

	MODPlay_Init(&play);
	MODPlay_SetMOD(&play, music_mod);
	MODPlay_SetVolume(&play,63,63);
	MODPlay_Start(&play);

	while(1) {
		WPAD_ScanPads();
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME) exit(0);

		GX_SetViewport(0,0, donwidth * 2, donheight * 4, 0, 1);
		GX_SetScissor(0,0, donwidth * 2, donheight * 4);
		VIDEO_ClearFrameBuffer(rmode, bin, COLOR_BLACK);

		GX_LoadProjectionMtx(donperspective, GX_PERSPECTIVE);

		guLookAt(view, &cam, &up, &look);

		GXLightObj lobj;
		guVector lpos = {2.0f, 2.0f, 2.0f};
		guVecMultiply(view, &lpos, &lpos);
		GX_InitLightPos(&lobj, lpos.x, lpos.y, lpos.z);
		GX_InitLightDistAttn(&lobj, 0.5f, 0.5f, GX_DA_MEDIUM);
		GX_InitLightColor(&lobj, LC_WHITE);
		GX_LoadLightObj(&lobj,GX_LIGHT0);

		GX_SetChanCtrl(GX_COLOR0A0, GX_ENABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT0, GX_DF_CLAMP, GX_AF_NONE);

		GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC);
		GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
		guMtxRotRad(model, 'x', (float)frame * 0.035f);
		guMtxRotRad(model2, 'z', (float)frame * 0.01f);
		guMtxConcat(model2, model, model);
		guMtxTransApply(model, model, 0.0f, 0.0f, -4.0f);
		guMtxConcat(view,model,model);
		GX_LoadPosMtxImm(model, GX_PNMTX0);
		GX_LoadNrmMtxImm(model, GX_PNMTX0);
		GX_SetCurrentMtx(GX_PNMTX0);

		GX_SetCullMode(GX_CULL_FRONT);
		GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);

		ag_config_Torus(0);
		ag_draw_Torus(0);

		GX_CopyDisp(capture_buffer, GX_TRUE);
		GX_DrawDone();

		u32 *ptr = (u32 *)capture_buffer;
		printf("\x1b[1;1H");
		for(int i = 0; i < donheight; i++) {
			for(int j = 0; j < donwidth; j++) {
				u16 lut_index = 0;
				u32 r_sum = 0, g_sum = 0, b_sum = 0;

				for(int py = 0; py < 4; py++) {
					for(int px = 0; px < 2; px++) {
						u8 img_x = (j * 2) + px;
						u8 img_y = (i * 4) + py;

						u32 pair = ptr[img_y * 320 + (img_x / 2)];

						u8 y1 = (pair >> 24) & 0xFF;
						u8 u  = (pair >> 16) & 0xFF;
						u8 y2 = (pair >> 8)  & 0xFF;
						u8 v  = (pair)       & 0xFF;

						u8 y = (img_x % 2 == 0) ? y1 : y2;

						int r_val = 0, g_val = 0, b_val = 0;

						if (y > 35) {
							int c = y - 16;
							int d = u - 128;
							int e = v - 128;

							r_val = (298 * c + 409 * e + 128) >> 8;
							g_val = (298 * c - 100 * d - 208 * e + 128) >> 8;
							b_val = (298 * c + 516 * d + 128) >> 8;
						}

						u8 r = (r_val < 0) ? 0 : (r_val > 255 ? 255 : r_val);
						u8 g = (g_val < 0) ? 0 : (g_val > 255 ? 255 : g_val);
						u8 b = (b_val < 0) ? 0 : (b_val > 255 ? 255 : b_val);

						u8 val = y >> 6;
						u8 shift = (py * 2 + px) * 2;
						lut_index |= (val << shift);

						r_sum += r; g_sum += g; b_sum += b;

					}
				}

				char c = (char)shape_lut_bin[lut_index];
				printf("\e[38;2;%i;%i;%im%c",
					   r_sum / 8, g_sum / 8, b_sum / 8, c);
			}
			printf("\n");
		}

		if(!frame) {
			VIDEO_SetBlack(false);
		}
		VIDEO_Flush();
		VIDEO_WaitVSync();

		frame++;
	}
}

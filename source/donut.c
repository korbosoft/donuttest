#include <gccore.h>
#include <stdio.h>
#include <math.h>
#include <grrlib.h>

#include "donut.h"
#include "grrproxy.h"

#include "shape_lut_bin.h"

u16 width = DONUT_WIDTH * 2;
GRRLIB_texImg *donutBuffer;

void draw_frosting(f32 minor, f32 major, int nsides, int rings, u32 col) {
    const f32 ringDelta = 2.0 * M_PI / rings;
    const f32 sideDelta = M_PI / nsides;
    const f32 waveAmp = 0.5f;
    const f32 waveFreq = 9.0f;
    minor += 0.1;
    major += 0.1;


    for (int i = 0; i < rings; i++) {
        f32 theta = i * ringDelta;
        f32 theta1 = theta + ringDelta;

        f32 cosTheta = cosf(theta), sinTheta = sinf(theta);
        f32 cosTheta1 = cosf(theta1), sinTheta1 = sinf(theta1);

        f32 cutZ0 = waveAmp * sinf(theta * waveFreq);
        f32 cutZ1 = waveAmp * sinf(theta1 * waveFreq);

        GX_Begin(GX_TRIANGLESTRIP, GX_VTXFMT0, 2 * (nsides + 1));
        for (int j = 0; j <= nsides; j++) {
            f32 progress = (f32)j / (f32)nsides;
            f32 phi = progress * M_PI;

            f32 curve = sqrt(1.0 - progress*progress);
            f32 scaledMinor = minor * curve;
            f32 scaledMajor = major * curve;
            f32 cosPhi = cosf(phi), sinPhi = sinf(phi);
            f32 dist = scaledMajor + scaledMinor * cosPhi;

            f32 z = minor * sinPhi;
            if (z < cutZ1) z = cutZ1;

            GX_Position3f32(cosTheta1 * dist, -sinTheta1 * dist, z);
            GX_Normal3f32(cosTheta1 * cosPhi, -sinTheta1 * cosPhi, sinPhi);
            GX_Color1u32(col);

            if (z < cutZ0) z = cutZ0;

            GX_Position3f32(cosTheta * dist, -sinTheta * dist, z);
            GX_Normal3f32(cosTheta * cosPhi, -sinTheta * cosPhi, sinPhi);
            GX_Color1u32(col);
        }
        GX_End();
    }
}

void donut_init(void) {
    donutBuffer = GRRLIB_CreateEmptyTexture(width + (width % 4), DONUT_HEIGHT * 4);
}

void donut_exit(void) {
    GRRLIB_FreeTexture(donutBuffer);
}

void draw_donut(float A, float B) {
    Mtx model, model2;

    guMtxRotRad(model, 'x', A);
    guMtxRotRad(model2, 'z', B);
    guMtxConcat(model2, model, model);
    guMtxTransApply(model, model, 0.0f, 0.0f, -(3.0f / sinf(DegToRad(DONUT_FOV) / 2.0f)));
    guMtxConcat(view,model,model);
    // load the modelview matrix into matrix memory
    GX_LoadPosMtxImm(model, GX_PNMTX0);
    GX_LoadNrmMtxImm(model, GX_PNMTX0);
    GX_SetCurrentMtx(GX_PNMTX0);
    GRRLIB_DrawTorus(1, 2, 64, 128, true, 0xFFFFFFFF);
    draw_frosting(1, 2, 64, 128, 0xFF00FFFF);

    GX_SetViewport(0,0, DONUT_WIDTH * 2, DONUT_HEIGHT * 4, 0, 1);
    GX_SetScissor(0,0, DONUT_WIDTH * 2, DONUT_HEIGHT * 4);
    GRRLIB_Screen2Texture(0, 0, donutBuffer, true);

    char frameBuffer[DONUT_WIDTH * DONUT_HEIGHT * 32];
    char *ptr = frameBuffer;

    printf("\x1b[0;0H");
    for(u8 i = 0; i < DONUT_HEIGHT; i++) {
        for(u8 j = 0; j < DONUT_WIDTH; j++) {
            u16 lutIndex = 0;
            u32 r_sum = 0, g_sum = 0, b_sum = 0;

            for(u8 py = 0; py < 4; py++) {
                for(u8 px = 0; px < 2; px++) {
                    u8 img_x = (j * 2) + px;
                    u8 img_y = (i * 4) + py;

                    u32 col = GRRLIB_GetPixelFromtexImg(img_x, img_y, donutBuffer);

                    u8 r = R(col), g = G(col), b = B(col);

                    u8 y = (r + g + b) / 3;

                    u8 val = (y >> 6) & 0x03;
                    u8 shift = (py * 2 + px) * 2;
                    lutIndex |= (val << shift);

                    r_sum += r; g_sum += g; b_sum += b;

                }
            }
            u8 r_avg = r_sum >> 3;
            u8 g_avg = g_sum >> 3;
            u8 b_avg = b_sum >> 3;
            char c = (char)shape_lut_bin[lutIndex];
            if ((r_avg + g_avg + b_avg) != 0) {
                ptr += sprintf(ptr, "\e[38;2;%i;%i;%im%c", r_avg, g_avg, b_avg, c);
            } else {
                ptr += sprintf(ptr, " ");
            }
        }
        ptr += sprintf(ptr, "\n");
    }
    puts(frameBuffer);
}

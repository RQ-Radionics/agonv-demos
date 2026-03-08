/*
 * mandelbrot.c - Mandelbrot set demo for ESP32-MOS
 *
 * Usage:  mandelbrot [mode]
 *   mode  VDP screen mode (default: 12 = 320x256 16 colours)
 *
 * The program reads the actual resolution and colour count from the VDP
 * sysvars after setting the mode, so it adapts automatically to any mode.
 *
 * VDU coordinate space is 2× pixel space on Agon, so:
 *   MAXX = scrWidth  * 2 - 1  (max VDU X)
 *   MAXY = scrHeight * 2 - 1  (max VDU Y)
 *   XSTP = MAXX / scrWidth    (= 2, step between VDU columns)
 *   YSTP = MAXY / scrHeight   (= 2, step between VDU rows)
 *
 * Build:  make -C mandelbrot
 * Run:    mandelbrot        (mode 12)
 *         mandelbrot 1      (mode 1 = 512x384 64 colours)
 *         mandelbrot 3      (mode 3 = 640x480 16 colours)
 */

#include <stdint.h>
#include "mos_api_table.h"

/* ── VDU helpers ─────────────────────────────────────────────────────────── */

static t_mos_api *g_mos;

static inline void vdu(uint8_t b)      { g_mos->putch(b); }
static inline void vdu16(int16_t v)    { vdu((uint8_t)(v & 0xFF)); vdu((uint8_t)((v >> 8) & 0xFF)); }

static void vdu_mode(uint8_t mode)     { vdu(22); vdu(mode); }
static void vdu_gcol(uint8_t colour)   { vdu(18); vdu(0); vdu(colour); }
static void vdu_plot_point(int x, int y)
{
    vdu(25); vdu(69); vdu16((int16_t)x); vdu16((int16_t)y);
}

/* ── Simple atoi (no stdlib) ─────────────────────────────────────────────── */
static int s_atoi(const char *s)
{
    int n = 0;
    while (*s >= '0' && *s <= '9') n = n * 10 + (*s++ - '0');
    return n;
}

/* ── Mandelbrot ──────────────────────────────────────────────────────────── */

__attribute__((section(".text.entry")))
int _start(int argc, char **argv, t_mos_api *mos)
{
    g_mos = mos;

    /* Parse mode argument (default 12) */
    uint8_t mode = 12;
    if (argc >= 2) mode = (uint8_t)s_atoi(argv[1]);

    /* Set mode and query actual resolution from VDP */
    vdu_mode(mode);
    mos->delay_ms(100);
    mos->vdp_request_mode();

    t_mos_sysvars *sv = mos->sysvars();
    int pw = sv->scrWidth;    /* pixel width  */
    int ph = sv->scrHeight;   /* pixel height */
    int nc = sv->scrColours;  /* number of colours */

    /* Clamp colours: use at most 64 for the gradient, at least 2 */
    int COLS  = nc > 64 ? 64 : (nc < 2 ? 2 : nc);
    int ITERS = 32;

    /* Agon VDU space is always 1280x1024 regardless of screen resolution.
     * Scale factor = VDU units per pixel. */
    int xscale = 1280 / pw;
    int yscale = 1024 / ph;

    /* Fix real range -2.5..+1.0 (width=3.5, centre=-0.75).
     * Derive imaginary range symmetrically from screen aspect ratio. */
    float ci_half = 3.5f * (float)ph / (float)pw / 2.0f;

    /* 4-pass interleave for progressive rendering */
    const int A[4] = {0, 1, 0, 1};
    const int B[4] = {0, 1, 1, 0};

    for (int i = 0; i < 4; i++) {
        for (int py = B[i]; py < ph; py += 2) {
            for (int px = A[i]; px < pw; px += 2) {

                float cr = -2.5f + (float)px * 3.5f / (float)(pw - 1);
                float ci = -ci_half + (float)py * (2.0f * ci_half) / (float)(ph - 1);

                float zr = 0.0f, zi = 0.0f;
                float zr2 = 0.0f, zi2 = 0.0f;
                int   it = 0;

                do {
                    float z1 = zr2 - zi2 + cr;
                    zi  = 2.0f * zr * zi + ci;
                    zr  = z1;
                    zr2 = zr * zr;
                    zi2 = zi * zi;
                    it++;
                } while (it < ITERS && zr2 + zi2 < 4.0f);

                int c = (COLS - 1) - it * (COLS - 1) / ITERS;
                if (c < 0) c = 0;

                vdu_gcol((uint8_t)c);
                /* VDU origin is bottom-left, so invert Y */
                vdu_plot_point(px * xscale, (ph - 1 - py) * yscale);
            }
        }
    }

    /* Wait for keypress then restore mode 1 */
    mos->getkey();
    vdu_mode(1);

    return 0;
}

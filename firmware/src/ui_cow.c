/* SPDX-License-Identifier: GPL-3.0-only */
/* COW FX pages: the FX / DLY / REV-CHO graphs as a pixel cow that reacts to the settings.
 * Drawn with cv_rect into the 240 x 100 graph strip, integer maths only, no framebuffer, no
 * tables beyond SINE. Redrawn every other frame (graph_signature adds the animation phase). */
#define COW_W 21
#define COW_K RGB(34, 30, 34)
#define COW_P RGB(255, 150, 170)
#define COW_Y RGB(255, 208, 64)
#define COW_BODY RGB(240, 238, 228)

static int32_t cow_sin(uint32_t phase256)              /* 0..255 -> -127..127 */
{
    return SINE[(phase256 & 255u) * 4u] >> 8;
}
/* A cow of 21 x 15 units (u px each). flip faces left; mouth is the open mouth's height in units;
 * bob lifts the head; tint dims every light part (ghosts and far-away echoes). */
static void cow_draw(int32_t x, int32_t y, int32_t u, uint32_t flip, uint32_t frame, int32_t mouth, int32_t bob,
                     uint32_t tint)
{
    uint16_t body = color_scale(COW_BODY, tint), pink = color_scale(COW_P, tint), horn = color_scale(COW_Y, tint);
    int32_t s1 = (int32_t)(frame & 1u);
#define CR(ox, oy, w, h, c) cv_rect(flip ? x + (COW_W - (ox) - (w)) * u : x + (ox) * u, y + (oy) * u, (w) * u, (h) * u, (c))
    CR(0, 4, 1, 5, COW_K);
    CR(0, 9, 2, 1, pink);                                           /* tail */
    CR(2 + s1, 11, 2, 4, body);
    CR(5 - s1, 11, 2, 4, body);                                     /* legs */
    CR(9 + s1, 11, 2, 4, body);
    CR(12 - s1, 11, 2, 4, body);
    CR(2 + s1, 14, 2, 1, COW_K);
    CR(5 - s1, 14, 2, 1, COW_K);
    CR(9 + s1, 14, 2, 1, COW_K);
    CR(12 - s1, 14, 2, 1, COW_K);                                   /* hooves */
    CR(1, 4, 13, 7, body);                                          /* body */
    CR(3, 5, 3, 3, COW_K);
    CR(8, 4, 2, 2, COW_K);
    CR(9, 8, 3, 2, COW_K);                                          /* spots */
    CR(7, 11, 3, 1, pink);                                          /* udder */
    CR(13, 2 + bob, 6, 6, body);
    CR(16, 5 + bob, 4, 3, pink);                                    /* head, muzzle */
    CR(14, bob, 1, 2, horn);
    CR(18, bob, 1, 2, horn);
    CR(13, 3 + bob, 1, 2, COW_K);
    CR(15, 3 + bob, 1, 1, COW_K);                                   /* ear, eye */
    CR(17, 6 + bob, 1, 1, COW_K);
    CR(19, 6 + bob, 1, 1, COW_K);                                   /* nostrils */
    if (mouth > 0)
        CR(16, 8 + bob, 4, mouth, COW_K);
#undef CR
}
static void cow_ring(int32_t cx, int32_t cy, int32_t r, uint16_t c)   /* an arc of a ring, right half only */
{
    int32_t a;
    if (r < 2)
        return;
    for (a = -64; a <= 64; a += 3)                              /* (a in 1/256 turns, -90 .. +90 degrees) */
        cv_pset(cx + ((cow_sin((uint32_t)a + 64u) * r) >> 7), cy + ((cow_sin((uint32_t)a) * r) >> 7), c);
}
/* the cow shouts on the beat while playing, now and then when idle */
static int32_t cow_mouth(const track_t *t)
{
    if (song.playing)
        return (t->seq_idx & 3u) == 0u && (fm1_ms % 250u) < 120u ? 3 : 0;
    return fm1_ms % 1400u < 220u ? 2 : 0;
}

/* FX page: DST flames, CHO clones, DLY ghosts, REV rings around one cow */
static void graph_cow_fx(const track_t *t, uint16_t c)
{
    uint32_t dist = (uint32_t)t->p[P_DIST], chor = (uint32_t)t->p[P_CHOR], dly = (uint32_t)t->p[P_DLY],
             rev = (uint32_t)t->p[P_REV], k, frame = (fm1_ms / 180u) & 1u;
    int32_t mouth = cow_mouth(t), x0 = 112, y0 = 38, bob = mouth ? (int32_t)((fm1_ms / 90u) & 1u) : 0;
    cv_rect(0, 85, 240, 1, C_LINE);
    for (k = 0; k < (dly + 22u) / 32u && k < 4u; k++)            /* echoes trail to the left, fainter each */
        cow_draw(x0 - 26 * (int32_t)(k + 1), y0 + 2, 2, 0, frame ^ (k & 1u), 0, 0, 150u - k * 36u);
    for (k = 0; k < chor / 43u && k < 3u; k++) {                  /* chorus: detuned twins that wobble */
        int32_t wob = (cow_sin((uint32_t)(fm1_ms / 14u) + k * 85u) * 4) >> 7;
        cow_draw(x0 + 8 + 9 * (int32_t)(k + 1), y0 - 7 * (int32_t)(k + 1) + wob, 2, 0, frame ^ 1u, 0, 0, 120u - k * 20u);
    }
    cow_draw(x0, y0, 2, 0, frame, mouth, bob, 255u);
    for (k = 0; k < dist / 16u; k++) {                           /* distortion: flames along the back */
        int32_t fh = 4 + (int32_t)((fm1_ms / 70u + k * 5u) % 6u) + (int32_t)(dist / 24u);
        cv_rect(x0 + 6 + (int32_t)k * 4, y0 + 8 - fh, 2, fh, (k + fm1_ms / 140u) & 1u ? TE_RED : C_AMB);
    }
    if (dist > 90u)
        cv_rect(x0 + 30, y0 + 6, 2, 2, TE_RED);                  /* angry eyes */
    for (k = 0; k < 3u; k++) {                                   /* reverb: rings leaving the mouth */
        int32_t r = (int32_t)(((fm1_ms / 14u) + k * 40u) % 110u);
        if (rev > k * 40u)
            cow_ring(x0 + 40, y0 + 14, r * (int32_t)(30u + rev) / 150, r < 60 ? c : C_DIM);
    }
    if (mouth)
        cv_text(x0 + 46, y0 - 14, &FONT_S, "MOO", C_WHITE);
}

/* DLY page: the cow bounces left and right; TIME sets the pace, FDBK how many ghosts follow */
static void graph_cow_dly(const track_t *t, uint16_t c)
{
    uint32_t ms = dly_samples((uint32_t)song.g[G_DTIME]) / 48u, ghosts = 1u + (uint32_t)song.g[G_DFDBK] / 24u, k;
    int32_t mix = song.g[G_DMIX];
    (void)t, (void)c;
    if (ms < 60u)
        ms = 60u;
    if (ms > 2000u)
        ms = 2000u;
    if (ghosts > 6u)
        ghosts = 6u;
    cv_rect(0, 88, 240, 2, C_LINE);
    for (k = ghosts; k-- > 0u;) {                                /* the oldest ghost first, the cow itself last */
        uint32_t tt = fm1_ms > k * ms ? fm1_ms - k * ms : 0u, ph = (tt % (ms * 2u)) * 256u / (ms * 2u);
        uint32_t leg = ph & 127u, back = ph >= 128u;
        int32_t px = 6 + (int32_t)(back ? 128u - leg : leg) * 160 / 128, lift = (cow_sin(leg) * 34) >> 7;
        uint32_t tint = k ? (uint32_t)(mix * 2) / (k + 1u) + 40u : 255u;
        if (tint > 255u)
            tint = 255u;
        cow_draw(px, 52 - (lift > 0 ? lift : 0), 2, back, (tt / 130u) & 1u, k == 0u && leg < 12u ? 2 : 0, 0, tint);
    }
}

/* REV/CHO page: a canyon; SIZE makes more, farther echo cows, CRATE / CDP make the cow wobble */
static void graph_cow_rev(const track_t *t, uint16_t c)
{
    uint32_t size = (uint32_t)song.g[G_RSIZE], damp = (uint32_t)song.g[G_RDAMP], k, crate = (uint32_t)song.g[G_CRATE],
             cdep = (uint32_t)song.g[G_CDEPTH];
    int32_t mouth = cow_mouth(t), x;
    int32_t wob = (cow_sin((uint32_t)(fm1_ms * (4u + crate / 8u) / 64u)) * (int32_t)(cdep / 24u)) >> 7;
    for (x = 0; x < 240; x += 4) {                               /* the canyon walls */
        int32_t h1 = 22 + (x * 7) % 31 + (x > 120 ? (x - 120) / 4 : 0), h2 = 10 + (x * 13) % 19;
        cv_rect(x, 90 - h1, 4, h1, C_LINE);
        cv_rect(x, 90 - h2, 4, h2, C_DIM);
    }
    cow_draw(6, 44 + wob, 2, 0, (fm1_ms / 200u) & 1u, mouth, mouth ? 1 : 0, 255u);
    for (k = 0; k < 1u + size / 34u && k < 4u; k++) {             /* echo cows on the far cliffs */
        uint32_t ph = (fm1_ms + 20000u - (600u + k * 260u)) % 1400u;
        int32_t ex = 100 + (int32_t)k * 36;
        int32_t tint = 230 - (int32_t)k * 45 - (int32_t)(damp / 4u);
        if (tint < 40)
            tint = 40;
        cow_draw(ex, 52 - (int32_t)k * 5, 1, 1, 0, ph < 240u ? 2 : 0, 0, (uint32_t)tint);
        if (ph < 240u)
            cv_text(ex + 2, 38 - (int32_t)k * 5, &FONT_S, k ? "moo" : "MOO", color_scale(C_WHITE, (uint32_t)tint));
    }
    for (k = 0; k < 3u; k++) {
        int32_t r = (int32_t)(((fm1_ms / 11u) + k * 36u) % 110u);
        cow_ring(48, 58 + wob, r, r < 50 ? c : C_DIM);
    }
}

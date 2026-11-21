/* Kit Per Bank for the Digitakt: a Modwerk elemod module.
 * The core calls the handlers named in build.json on its shared hooks, in order
 * with other mods. Handlers use the C calling convention: arguments on the
 * stack, d0-d1/a0-a1 free, the result in d0.
 * Never copy firmware bytes into source: refer to the stock OS only through
 * the core's documented symbols.
 */

#include <stdint.h>

/* ---- firmware addresses (1.53 / 1.54 via -DOS154 cflags define) ---------- */
#ifdef OS154
#define F_FILLRECT   0x400c1bce
#define F_FRAMERECT  0x400c19b2
#define F_TEXTF      0x400c27a4
#define F_INVALIDATE 0x400c9a3a
#define F_BLIT       0x400c2b88
#define F_CHECKBOXES 0x421f8a3c
#define F_FONT5      0x40200ebc
#define F_ISHELD     0x400d399c
#define F_PATTERN    0x4197c6c7
#else
#define F_FILLRECT   0x400c19a6
#define F_FRAMERECT  0x400c178a
#define F_TEXTF      0x400c257c
#define F_INVALIDATE 0x400c9812
#define F_BLIT       0x400c2960
#define F_CHECKBOXES 0x421f7a3c
#define F_FONT5      0x40200b0c
#define F_ISHELD     0x400d3774
#define F_PATTERN    0x4197b6c7
#endif

#define PATTERN    (*(volatile unsigned char *)F_PATTERN)
#define INVALIDATE ((void (*)(void *))F_INVALIDATE)
#define FILLRECT   ((void (*)(void *, int, int, int, int, int))F_FILLRECT)
#define FRAMERECT  ((void (*)(void *, int, int, int, int, int))F_FRAMERECT)
#define TEXT       ((void (*)(void *, const void *, int, int, int, \
                               const char *, ...))F_TEXTF)
#define FONT5      ((const void *)F_FONT5)
#define BLIT       ((void (*)(void *, const void *, int, int, int))F_BLIT)
#define KEYHELD(k) (((uint32_t (*)(int))F_ISHELD)(k))
#define KEYID(ev)      (*(int *)((char *)(ev) + 12))
#define KEYPRESS(ev)   ((*(int *)((char *)(ev) + 16) & 9) == 1)

#define KEY_PTN 3
#define KEY_YES 12

/* ---- state --------------------------------------------------------------- */
static unsigned char  kpb_enabled;        /* KIT PER BANK on/off */
static unsigned short kpb_lock[8];        /* bit i of kpb_lock[bank]: pattern i locked */
static const char    *kpb_msg;            /* OSD message text */
static volatile int   kpb_msg_t;          /* ticks remaining for OSD (30 Hz) */

static int is_locked(int pat)
{
    return (kpb_lock[pat >> 4] >> (pat & 15)) & 1;
}

/* ---- ev_settings row callbacks (called from glue.s via kpb_row) ---------- */

void kpb_row_select(void **payload)
{
    kpb_enabled ^= 1;
    INVALIDATE((char *)*payload + 0x38);
}

void kpb_row_draw(void *a, void *b, void *bmp, int x, int y)
{
    (void)a; (void)b;
    BLIT(bmp, (const void *)(F_CHECKBOXES + (kpb_enabled ? 0x1c : 0)), x, y, 0);
}

void kpb_row_change(void **payload, int unused, int delta)
{
    (void)unused;
    int on = delta > 0 ? 1 : 0;
    if (on == (int)kpb_enabled)
        return;
    kpb_enabled = (unsigned char)on;
    INVALIDATE((char *)*payload + 0x38);
}

/* ---- ev_draw: OSD toast --------------------------------------------------- */

void kpb_draw(void *bmp, void *ctrl)
{
    (void)ctrl;
    int t = kpb_msg_t;
    if (t <= 0)
        return;
    kpb_msg_t = t - 1;
    FILLRECT(bmp, 2, 52, 125, 62, 0);       /* clear: black background */
    FRAMERECT(bmp, 2, 52, 125, 62, -1);     /* white border */
    TEXT(bmp, FONT5, 6, 55, -1, kpb_msg);
}

/* ---- ev_key --------------------------------------------------------------- */

int kpb_key(void *brain, void *ev)
{
    (void)brain;
    if (!kpb_enabled)       return 0;
    if (!KEYPRESS(ev))      return 0;
    if (KEYID(ev) != KEY_YES) return 0;
    if (!KEYHELD(KEY_PTN))  return 0;

    int pat  = PATTERN;
    int bank = pat >> 4;
    int pos  = pat & 15;

    if (pos == 0) {
        kpb_msg   = "BANK KIT";
        kpb_msg_t = 45;
        return 1;
    }

    kpb_lock[bank] ^= (1u << pos);
    int locked = (kpb_lock[bank] >> pos) & 1;
    kpb_msg   = locked ? "KIT LOCKED" : "KIT SHARED";
    kpb_msg_t = 45;
    return 1;
}

/* ---- ev_pattern_change (pending hook, subscribed once it ships) ----------- */

int kpb_pattern_change(int new_pat, int old_pat)
{
    (void)old_pat;
    if (!kpb_enabled)      return new_pat;
    int pos = new_pat & 15;
    if (pos == 0)          return new_pat;  /* first pattern: IS the bank kit */
    if (is_locked(new_pat)) return new_pat; /* locked: own kit */
    return new_pat & ~15;                   /* shared: bank's first pattern */
}

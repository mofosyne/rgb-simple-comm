/**
  Title: RGB Simple Communication - Human Readable Approach
  Author: Brian Khuu

  Designed for human observers. A field technician can watch the LED and call
  out the colour sequence verbally, or transcribe it for later decoding.

  Key differences from the machine-readable (dark-clocked) approach:
    - Cyan excluded: too easily called "light blue", and can visually pass
      for blue on a real LED at distance.
    - Data colours chosen for verbal distinctiveness: Blue, Magenta, Yellow,
      White. No two are likely to be confused in speech.
    - Green and Red serve as idle indicators AND byte markers. When idle the
      LED holds steady green (normal) or red (fault). At each byte boundary
      the same colours appear as a pulse, doubling as parity indicator.

  Colour table:
  | COLOUR  | R | G | B | Meaning                            |
  |---------|---|---|---|------------------------------------|
  | DARK    | 0 | 0 | 0 | Separator between pulses           |
  | BLUE    | 0 | 0 | 1 | Data: 00                           |
  | GREEN   | 0 | 1 | 0 | Idle (normal) / byte mark parity=0 |
  | CYAN    | 0 | 1 | 1 | (unused)                           |
  | RED     | 1 | 0 | 0 | Idle (fault)  / byte mark parity=1 |
  | MAGENTA | 1 | 0 | 1 | Data: 01                           |
  | YELLOW  | 1 | 1 | 0 | Data: 10                           |
  | WHITE   | 1 | 1 | 1 | Data: 11                           |
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef enum {
    DARK    = 0,
    BLUE    = 1,
    GREEN   = 2,
    CYAN    = 3,
    RED     = 4,
    MAGENTA = 5,
    YELLOW  = 6,
    WHITE   = 7
} rgb_colour_t;

typedef enum { NO_PARITY, EVEN_PARITY, ODD_PARITY } paritySel_t;

#define PARITY_SETTING ODD_PARITY
#define SEQ_MAX 200

static uint8_t calcParity(uint8_t data, paritySel_t sel) {
    uint8_t p = 0;
    while (data) { p ^= data & 1; data >>= 1; }
    if (sel == ODD_PARITY)  return (~p) & 1;
    if (sel == EVEN_PARITY) return p & 1;
    return 1;
}

static uint8_t validParity(uint8_t data, uint8_t parity_bit, paritySel_t sel) {
    return (calcParity(data, sel) & 1) == (parity_bit & 1);
}


/*
  ENCODE
*/

static rgb_colour_t bits2_to_colour(uint8_t b) {
    switch (b & 0x3) {
        case 0: return BLUE;
        case 1: return MAGENTA;
        case 2: return YELLOW;
        case 3: return WHITE;
    }
    return DARK;
}

void encode_uint8(uint8_t data, rgb_colour_t *seq, int *pos) {
    for (int i = 3; i >= 0; i--) {
        seq[(*pos)++] = DARK;
        seq[(*pos)++] = bits2_to_colour((data >> (i * 2)) & 0x3);
    }
    seq[(*pos)++] = DARK;
    seq[(*pos)++] = (calcParity(data, PARITY_SETTING) == 0) ? GREEN : RED;
}


/*
  DECODE

  colour_to_result maps an incoming colour to an action:
    0       - data bits written to *bits_out
    1       - byte mark GREEN received (parity bit = 0 / normal)
    2       - byte mark RED  received (parity bit = 1 / fault)
   -1       - DARK separator, caller should skip
   -2       - unexpected colour (CYAN or unknown)
*/
static int colour_to_result(rgb_colour_t c, uint8_t *bits_out) {
    switch (c) {
        case BLUE:    *bits_out = 0; return 0;
        case MAGENTA: *bits_out = 1; return 0;
        case YELLOW:  *bits_out = 2; return 0;
        case WHITE:   *bits_out = 3; return 0;
        case GREEN:   return 1;
        case RED:     return 2;
        case DARK:    return -1;
        default:      return -2;  // CYAN or unknown
    }
}

/*
  Returns:
    1 or 2  - byte received (GREEN=1 parity=0, RED=2 parity=1)
   -1       - end of stream or unexpected colour
*/
int decode_uint8(const rgb_colour_t *seq, int seq_len, int *pos, uint8_t *output) {
    *output = 0;
    int bits_received = 0;

    while (*pos < seq_len) {
        rgb_colour_t c = seq[*pos];
        (*pos)++;

        uint8_t bits = 0;
        int r = colour_to_result(c, &bits);

        if (r == -1) continue;
        if (r == -2) return -1;

        if (r == 1 || r == 2) {
            if (bits_received == 4) return r;
            return -1;
        }

        *output = (*output << 2) | bits;
        bits_received++;
    }
    return -1;
}


/*
  TEST TOOLS
*/

static const char *colour_str[] = {
    "Dark","Blue","Green","Cyan","Red","Magenta","Yellow","White"
};

static const char *colour_short[] = { "D","B","G","C","R","M","Y","W" };

static const char *colour_marked[] = {
    "_D_"," B ","|G|"," C ","|R|"," M "," Y "," W "
};

static uint8_t printByte(uint8_t b) {
    printf(" %x '%c' = ", b, b);
    for (int i = 7; i >= 0; i--) printf("%d", (b >> i) & 1);
    printf("\n");
    return b;
}

int main(void) {
    printf("Human-Readable RGB Comm Test\n============================\n");

    rgb_colour_t seq[SEQ_MAX];
    memset(seq, 0, sizeof(seq));
    int j = 0;

    printf("\n# ENCODING Input\n");
    const char *msg = "HELLO WORLD...    ";
    for (int i = 0; msg[i]; i++)
        encode_uint8(printByte((uint8_t)msg[i]), seq, &j);

    printf("\n# Encoded Colour Sequence Output\n");
    for (int i = 0; i < j; i++)
        printf("%d:%s ", i, colour_str[seq[i]]);

    printf("\n\n# Encoded Colour Sequence Output (compact)\n");
    for (int i = 0; i < j; i++)
        printf("%s", colour_short[seq[i]]);

    printf("\n\n# Encoded Colour Sequence Output (marked)\n");
    for (int i = 0; i < j; i++)
        printf("%s", colour_marked[seq[i]]);

    printf("\n\n# DECODING Test\n");
    int k = 0;
    uint8_t out;
    while (k < j) {
        int rc = decode_uint8(seq, j, &k, &out);
        if (rc < 0) {
            printf(" [END OF TRANSMISSION] ");
            break;
        }
        printf("%c", out);
        uint8_t parity_bit = (rc == 2) ? 1 : 0;
        if (!validParity(out, parity_bit, PARITY_SETTING)) printf("! ");
    }

    printf("\n\n# Completed\n");
    return 0;
}

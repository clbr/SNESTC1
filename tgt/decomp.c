/*  SNES tile compression
    Copyright (C) 2023 Lauri Kasanen

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <string.h>
#include <stdint.h>

typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;

void tochr(const u8 *in, u8 * const out);

enum {
	M_FLAT,
	M_1BIT,
	M_2BIT,
	M_3BIT,
	M_RLE,
	M_HLINE,
	M_VLINE,
	M_COMMONBYTE,
	M_ANCESTOR,
	M_UNCOMPRESSED,

	NUM_METHODS,
};

u8 decomp_flat(const u8 *in, u8 *out);

u8 decomp_1bit(const u8 *in, u8 *out);

u8 decomp_2bit(const u8 *in, u8 *out);

static u8 decomp_3bit(const u8 *in, u8 *out) {
	u8 tab[8], i, cur = 0, j;
	u16 used;
	u32 val = 0;
	memcpy(&used, in, 2);
	in += 2;

	for (i = 0; i < 16; i++) {
		if (used & (1 << i))
			tab[cur++] = i;
	}

	for (i = 0; i < 16; i += 2) {
		memcpy(&val, in, 3);
		in += 3;

		for (j = 0; j < 16; j += 2) {
			*out++ = tab[val & 7];
			val >>= 3;
		}
	}

	return 26;
}

static u8 decomp_rle(const u8 *in, u8 *out) {
	const u8 * const orig = in;
	u8 wrote = 0, cur, buf, n = 0, prev, i, run;

	prev = buf = *in++;
	prev &= 15;
	buf >>= 4;
	n = 1;

	while (1) {
		#define get() \
		if (!n) { \
			buf = *in++; \
			cur = buf & 15; \
			buf >>= 4; \
			n = 1; \
		} else { \
			cur = buf & 15; \
			n = 0; \
		}

		get();

		if (cur != prev) {
			*out++ = prev;
			wrote++;

			if (wrote == 63) {
				*out++ = cur;
				wrote++;
				break;
			}
			prev = cur;
		} else {
			get();
			run = cur;
			while (cur == 15) {
				get();
				run += cur;
			}
			run += 2;

			for (i = 0; i < run; i++) {
				*out++ = prev;
			}
			wrote += run;
			if (wrote == 64)
				break;

			get();
			prev = cur;
			if (wrote == 63) {
				*out++ = cur;
				wrote++;
				break;
			}
		}

		#undef get
	}

	return in - orig;
}

static u8 decomp_hline(const u8 *in, u8 *out) {
	const u8 * const orig = in;

	const u8 val = *in++;
	u8 highline[4], i;

	memcpy(highline, in, 4);
	in += 4;

	for (i = 0; i < 8; i++) {
		if (val & (1 << i)) {
			*out++ = highline[0] & 15;
			*out++ = highline[0] >> 4;
			*out++ = highline[1] & 15;
			*out++ = highline[1] >> 4;
			*out++ = highline[2] & 15;
			*out++ = highline[2] >> 4;
			*out++ = highline[3] & 15;
			*out++ = highline[3] >> 4;
		} else {
			*out++ = in[0] & 15;
			*out++ = in[0] >> 4;
			*out++ = in[1] & 15;
			*out++ = in[1] >> 4;
			*out++ = in[2] & 15;
			*out++ = in[2] >> 4;
			*out++ = in[3] & 15;
			*out++ = in[3] >> 4;

			in += 4;
		}
	}

	return in - orig;
}

static u8 decomp_vline(const u8 *in, u8 *out) {
	const u8 * const orig = in;

	const u8 val = *in++;
	u8 highline[4], i;

	memcpy(highline, in, 4);
	in += 4;

	for (i = 0; i < 8; i++) {
		if (val & (1 << i)) {
			out[0 * 8 + i] = highline[0] & 15;
			out[1 * 8 + i] = highline[0] >> 4;
			out[2 * 8 + i] = highline[1] & 15;
			out[3 * 8 + i] = highline[1] >> 4;
			out[4 * 8 + i] = highline[2] & 15;
			out[5 * 8 + i] = highline[2] >> 4;
			out[6 * 8 + i] = highline[3] & 15;
			out[7 * 8 + i] = highline[3] >> 4;
		} else {
			out[0 * 8 + i] = in[0] & 15;
			out[1 * 8 + i] = in[0] >> 4;
			out[2 * 8 + i] = in[1] & 15;
			out[3 * 8 + i] = in[1] >> 4;
			out[4 * 8 + i] = in[2] & 15;
			out[5 * 8 + i] = in[2] >> 4;
			out[6 * 8 + i] = in[3] & 15;
			out[7 * 8 + i] = in[3] >> 4;

			in += 4;
		}
	}

	return in - orig;
}

static u8 decomp_commonbyte(const u8 *in, u8 *out) {
	const u8 * const orig = in;
	u8 i;

	const u8 cur = *in++;
	u32 bits;
	memcpy(&bits, in, 4);
	in += 4;

	for (i = 0; i < 32; i++) {
		if (bits & (1UL << i))
			*out++ = cur;
		else
			*out++ = *in++;
	}

	return in - orig;
}

static u8 decomp_ancestor(const u8 *in, u8 *out, u8 summary) {
	const u8 *src = out;
	const u8 * const origin = in;
	const u8 *val;
	u8 i, s;

	u16 dist = 0;
	do {
		i = *in++;
		dist += i;
	} while (i == 255);
	dist++;
	src -= 32 * dist;

	memcpy(out, src, 32);

	val = in;

	if (summary & 1)
		in++;
	if (summary & 2)
		in++;
	if (summary & 4)
		in++;
	if (summary & 8)
		in++;

	for (s = 0; s < 4; s++) {
		if (summary & 1 << s) {
			for (i = 0; i < 8; i++) {
				if (*val & 1 << i)
					*out++ = *in++;
				else
					out++;
			}
			val++;
		} else {
			out += 8;
		}
	}

	return in - origin;
}

static u8 decomp_uncompressed(const u8 *in, u8 *out) {
	memcpy(out, in, 32);
	return 32;
}

typedef u8 (*mfunc)(const u8 *in, u8 *out);

static const mfunc methods[NUM_METHODS] = {
	decomp_flat,
	decomp_1bit,
	decomp_2bit,
	decomp_3bit,
	decomp_rle,
	decomp_hline,
	decomp_vline,
	decomp_commonbyte,
	NULL,
	decomp_uncompressed,
};

void stc1_decompress(const u8 *in, u8 *out) {

	u8 tmp[128];
	u16 numtiles, t;
	memcpy(&numtiles, in, 2);
	in += 2;

	for (t = 0; t < numtiles; t++) {
		const u8 mbyte = *in++;
		const u8 m = mbyte & 15;

		if (m >= M_COMMONBYTE) {
			// already in CHR format
			if (m == M_ANCESTOR)
				in += decomp_ancestor(in, out, mbyte >> 4);
			else
				in += methods[m](in, out);
		} else if (m == M_FLAT) {
			methods[M_FLAT](&mbyte, tmp);
		} else {
			in += methods[m](in, tmp);
		}

		if (m == M_VLINE || m == M_HLINE) {
			const u8 rot = mbyte >> 4;
			memcpy(tmp + 64, tmp, 64);
			tochr(tmp + 64 - rot, out);
		} else if (m < M_HLINE) {
//			if (mbyte >> 4 && m != M_FLAT)
//				die("Bug\n");
			tochr(tmp, out);
		}

		out += 32;
	}
}

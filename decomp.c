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

#include "main.h"

static u8 decomp_flat(const u8 *in, u8 *out) {
	memset(out, *in >> 4, 64);
	return 0;
}

static u8 decomp_1bit(const u8 *in, u8 *out) {
	const u8 flood = *in & 15;
	const u8 other = *in >> 4;
	in++;

	u8 i, j;
	for (i = 0; i < 8; i++) {
		u8 val = in[i];
		for (j = 0; j < 8; j++) {
			if (val & 1)
				*out++ = other;
			else
				*out++ = flood;
			val >>= 1;
		}
	}

	return 9;
}

static u8 decomp_2bit(const u8 *in, u8 *out) {
	u16 used, val;
	memcpy(&used, in, 2);
	in += 2;

	u8 tab[4], i, cur = 0, j;
	for (i = 0; i < 16; i++) {
		if (used & (1 << i))
			tab[cur++] = i;
	}
	for (i = 0; i < 16; i += 2) {
		memcpy(&val, in, 2);
		in += 2;
		for (j = 0; j < 16; j += 2) {
			*out++ = tab[val & 3];
			val >>= 2;
		}
	}

	return 18;
}

static u8 decomp_3bit(const u8 *in, u8 *out) {
	u16 used;
	u32 val = 0;
	memcpy(&used, in, 2);
	in += 2;

	u8 tab[8], i, cur = 0, j;
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

static u8 decomp_hline(const u8 *in, u8 *out) {
	const u8 * const orig = in;

	const u8 val = *in++;
	u8 highline[4];

	memcpy(highline, in, 4);
	in += 4;

	u8 i;
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
	u8 highline[4];

	memcpy(highline, in, 4);
	in += 4;

	u8 i;
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

	const u8 cur = *in++;
	u32 bits;
	memcpy(&bits, in, 4);
	in += 4;

	u8 i;
	for (i = 0; i < 32; i++) {
		if (bits & (1 << i))
			*out++ = cur;
		else
			*out++ = *in++;
	}

	return in - orig;
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
	decomp_hline,
	decomp_vline,
	decomp_commonbyte,
	decomp_uncompressed,
};

void stc1_decompress(const u8 *in, u8 *out) {

	u8 tmp[128];
	u16 numtiles, t;
	memcpy(&numtiles, in, 2);
	in += 2;

	for (t = 0; t < numtiles; t++) {
		const u8 mbyte = *in++;
		const u8 m = mbyte & 7;

		if (m >= M_COMMONBYTE) {
			// already in CHR format
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
			if (mbyte >> 4 && m != M_FLAT)
				die("Bug\n");
			tochr(tmp, out);
		}

		out += 32;
	}
}

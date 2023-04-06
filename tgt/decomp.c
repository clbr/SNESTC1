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
u8 decomp_3bit(const u8 *in, u8 *out);
u8 decomp_rle(const u8 *in, u8 *out);
u8 decomp_hline(const u8 *in, u8 *out);
u8 decomp_vline(const u8 *in, u8 *out);
u8 decomp_commonbyte(const u8 *in, u8 *out);
u8 decomp_ancestor(const u8 *in, u8 *out, u8 summary);
u8 decomp_uncompressed(const u8 *in, u8 *out);

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

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
#include <png.h>
#include <stdarg.h>

void die(const char fmt[], ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	exit(1);
}

static u32 readpng(FILE *f, u8 *data) {
	const u8 * const orig = data;

	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);
	if (!png_ptr) die("PNG error\n");
	png_infop info = png_create_info_struct(png_ptr);
	if (!info) die("PNG error\n");
	if (setjmp(png_jmpbuf(png_ptr))) die("PNG error\n");

	png_init_io(png_ptr, f);
	png_read_png(png_ptr, info,
		PNG_TRANSFORM_PACKING|PNG_TRANSFORM_STRIP_16|PNG_TRANSFORM_STRIP_ALPHA, NULL);

	u8 **rows = png_get_rows(png_ptr, info);
	const u32 imgw = png_get_image_width(png_ptr, info);
	const u32 imgh = png_get_image_height(png_ptr, info);
	const u8 type = png_get_color_type(png_ptr, info);
	const u8 depth = png_get_bit_depth(png_ptr, info);

	if (imgw % 8 != 0 || imgh % 8 != 0)
		die("Image is not divisible by 8\n");

	if (type != PNG_COLOR_TYPE_PALETTE)
		die("Input must be a paletted PNG, got %u\n", type);

	if (depth != 8)
		die("Depth not 8 (%u) - maybe you have old libpng?\n", depth);

	const u32 rowbytes = png_get_rowbytes(png_ptr, info);
	if (rowbytes != imgw)
		die("Packing failed, row was %u instead of %u\n", rowbytes, imgw);

	u16 tx, ty;
	for (ty = 0; ty < imgh; ty += 8) {
		for (tx = 0; tx < imgw; tx += 8) {
			u8 y;
			for (y = 0; y < 8; y++) {
				memcpy(data, &rows[ty + y][tx], 8);
				data += 8;
			}
		}
	}

	fclose(f);
	png_destroy_info_struct(png_ptr, &info);
	png_destroy_read_struct(&png_ptr, NULL, NULL);

	return data - orig;
}

int main(int argc, char **argv) {

	u8 in[USHRT_MAX * 2], comp[USHRT_MAX], decomp[USHRT_MAX * 2],
		chr[USHRT_MAX];

	char namebuf[PATH_MAX];
	const char *name;

	if (argc < 2) {
		printf("Usage: %s in.png [out]\n", argv[0]);
		return 1;
	}

	if (argc == 2) {
		sprintf(namebuf, "%s.bin", argv[1]);
		name = namebuf;
	} else {
		name = argv[2];
	}

	FILE *f = fopen(argv[1], "r");
	if (!f) {
		die("Can't open file %s\n", argv[1]);
	}

	const u32 inlen = readpng(f, in);
	u32 i, c;
	for (i = 0, c = 0; i < inlen; i += 64, c += 32) {
		tochr(&in[i], &chr[c]);
	}

	const u32 complen = stc1_compress(in, comp, inlen);
	printf("Compressed to %u (%.2f%%)\n", complen, complen * 100.0f / (inlen / 2));

	stc1_decompress(comp, decomp);
	if (memcmp(chr, decomp, inlen / 2)) {
		for (c = 0; c < inlen / 64; c++) {
			if (memcmp(&chr[c * 32], &decomp[c * 32], 32)) {
				printf("Mismatch at tile %u\n", c);
				printf("Expected:\n");
				for (i = 0; i < 32; i++) {
					printf("%02x,", chr[c * 32 + i]);
					if (i % 8 == 7)
						puts("");
				}
				printf("Got:\n");
				for (i = 0; i < 32; i++) {
					printf("%02x,", decomp[c * 32 + i]);
					if (i % 8 == 7)
						puts("");
				}
			}
		}

		die("Decomp fail\n");
	}

	f = fopen(name, "w");
	if (!f) {
		die("Can't open output %s\n", name);
	}

	fwrite(comp, 1, complen, f);

	fclose(f);

	return 0;
}

/* Authored by notaz@openpandora.org */

#include <stdio.h>
#include <png.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/fb.h>


int main(int argc, char *argv[])
{
	struct fb_var_screeninfo fbvar;
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	png_bytepp row_ptr = NULL;
	size_t destsize;
	void *dest;
	int fd;
	FILE *fp;
	int ret;

	if (argc != 2)
	{
		fprintf(stderr, "usage: %s <png_file>\n", argv[0]);
		return 1;
	}

	fp = fopen(argv[1], "rb");
	if (fp == NULL)
	{
		perror("failed to open");
		return 1;
	}

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
	{
		fprintf(stderr, "png_create_read_struct() failed\n");
		fclose(fp);
		return 1;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		fprintf(stderr, "png_create_info_struct() failed\n");
		return 1;
	}

	// Start reading
	png_init_io(png_ptr, fp);
	png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_STRIP_ALPHA | PNG_TRANSFORM_PACKING, NULL);
	row_ptr = png_get_rows(png_ptr, info_ptr);
	if (row_ptr == NULL)
	{
		fprintf(stderr, "png_get_rows() failed\n");
		return 1;
	}

	printf("%s: %ix%i @ %ibpp\n", argv[1], (int)info_ptr->width, (int)info_ptr->height, info_ptr->pixel_depth);

	// mmap
	fd = open("/dev/fb0", O_RDWR);
	if (fd == -1)
	{
		perror("open(\"/dev/fb0\") failed");
		return 1;
	}

	ret = ioctl(fd, FBIOGET_VSCREENINFO, &fbvar);
	if (ret == -1)
		perror("ioctl(FBIOGET_VSCREENINFO)");

	destsize = fbvar.xres_virtual * fbvar.yres_virtual
		 * info_ptr->pixel_depth / 8;
	dest = mmap(0, destsize, PROT_WRITE|PROT_READ, MAP_SHARED, fd, 0);
	if (dest == MAP_FAILED)
	{
		perror("mmap(fbptr) failed");
		return 1;
	}

	fbvar.bits_per_pixel = info_ptr->pixel_depth;

	ret = ioctl(fd, FBIOPUT_VSCREENINFO, &fbvar);
	if (ret == -1)
		perror("ioctl(FBIOPUT_VSCREENINFO)");

	memset(dest, 0, destsize);

	switch (info_ptr->pixel_depth)
	{
		case 24:
		{
			int height, width, h;
			unsigned char *dst = dest;

			height = info_ptr->height;
			if (height > fbvar.yres_virtual)
				height = fbvar.yres_virtual;
			width = info_ptr->width;
			if (width  > fbvar.xres_virtual)
				width = fbvar.xres_virtual;
			dst += (fbvar.xres_virtual-width) / 2 * 3;

			for (h = 0; h < height; h++)
			{
				unsigned char *src = row_ptr[h];
				int len = width;
				while (len--)
				{
					*dst++ = src[2];
					*dst++ = src[1];
					*dst++ = src[0];
					src += 3;
				}
				dst += fbvar.xres_virtual - width;
			}
			break;
		}

		default:
			fprintf(stderr, "failed, can only handle 24bpp\n");
			break;
	}

	munmap(dest, destsize);
	close(fd);
	png_destroy_read_struct(&png_ptr, info_ptr ? &info_ptr : NULL, (png_infopp)NULL);
	fclose(fp);

	return 0;
}

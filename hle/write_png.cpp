
#include "write_png.h"

#include <png.h>
#include <stdio.h>

// struct file_guard;
// struct PNG_write;
// struct PNG_info;

struct file_guard {
    FILE* f;

    file_guard(FILE* f) : f(f) {}
    ~file_guard() { if (f) fclose(f); }

    // no copy
    file_guard(const file_guard&) = delete;
};

struct PNG_write {
    png_structp ptr;

    PNG_write() {
        ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    }

    ~PNG_write() {
        if (ptr) png_destroy_write_struct(&ptr, nullptr);
    }

    // no copy
    PNG_write(const PNG_write&) = delete;
};

struct PNG_info {
    png_infop ptr;

    PNG_info(PNG_write &png) {
        png_ptr = png.ptr;
        ptr = png_create_info_struct(png.ptr);
    }

    ~PNG_info() {
        if (ptr) png_free_data(png_ptr, ptr, PNG_FREE_ALL, -1);
    }

    // no copy
    PNG_info(const PNG_info&) = delete;
private:
    png_structp png_ptr;
};

bool writeImage(std::string filename, int width, int height, std::vector<uint32_t> data) {
    file_guard f(fopen(filename.c_str(), "wb"));
    if (f.f == nullptr) {
        fprintf(stderr, "Error opening to %s\n", filename.c_str());
        return false;
    }

    PNG_write png;
    if (png.ptr == nullptr){
        fprintf(stderr, "Could not allocate png_structp for %s\n", filename.c_str());
        return false;
    }

    PNG_info info(png);
    if (info.ptr == nullptr) {
        fprintf(stderr, "Could not allocate info structure for %s\n", filename.c_str());
        return false;
    }

    // Exception handling
    if (setjmp(png_jmpbuf(png.ptr))) {
		fprintf(stderr, "Error during png creation\n");
		return false;
    }

    png_init_io(png.ptr, f.f);

    // write header
    png_set_IHDR(png.ptr, info.ptr, width, height, 8,
        PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_write_info(png.ptr, info.ptr);

    for (uint32_t y = 0; y < height; y++) {
        uint32_t *row_ptr = data.data() + (y * width);
        png_write_row(png.ptr, (uint8_t*) row_ptr);
    }

    png_write_end(png.ptr, nullptr);

    return true;
}

#include "Image.h"

#include <cstdint>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>


/*
-------------------------------------------------------------------------------------------------------
Default colors definitions
-------------------------------------------------------------------------------------------------------
*/

const Color Color::Black		= Color(  0,  0,    0, 255);
const Color Color::White		= Color(255, 255, 255, 255);
const Color Color::Red			= Color(255,   0,   0, 255);
const Color Color::Green		= Color(  0, 255,   0, 255);
const Color Color::Blue			= Color(  0,   0, 255, 255);
const Color Color::Yellow		= Color(255, 255,   0, 255);
const Color Color::Cyan			= Color(  0, 255, 255, 255);
const Color Color::Purple		= Color(255,   0, 255, 255);
const Color Color::Gray			= Color(127, 127, 127, 255);
const Color Color::Orange		= Color(255, 127,   0, 255);
const Color Color::Transparent	= Color(  0,   0,   0,   0);

/*
-------------------------------------------------------------------------------------------------------
Helper functions to read and write BMPs
-------------------------------------------------------------------------------------------------------
*/

static inline void write_le16(FILE* f, uint16_t v) 
{
    uint8_t b[2] = { (uint8_t)(v & 0xFF), (uint8_t)((v >> 8) & 0xFF) };
    if (fwrite(b, 1, 2, f) != 2) 
        throw "Failed to write in file, possible corruption";
}
static inline void write_le32(FILE* f, uint32_t v) 
{
    uint8_t b[4] = {
        (uint8_t)(v & 0xFF),
        (uint8_t)((v >> 8) & 0xFF),
        (uint8_t)((v >> 16) & 0xFF),
        (uint8_t)((v >> 24) & 0xFF)
    };
    if (fwrite(b, 1, 4, f) != 4) throw "Failed to write in file, possible corruption";
}
static inline uint16_t read_le16(FILE* f) 
{
    uint8_t b[2]; 
    if (fread(b, 1, 2, f) != 2) 
        throw "Failed to read from file, possible corruption";

    return (uint16_t)(b[0] | (b[1] << 8));
}
static inline uint32_t read_le32(FILE* f) 
{
    uint8_t b[4]; 
    if (fread(b, 1, 4, f) != 4) 
        throw "Failed to read from file, possible corruption";

    return (uint32_t)(b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24));
}

/*
-------------------------------------------------------------------------------------------------------
Constructors / Destructors
-------------------------------------------------------------------------------------------------------
*/

// Initializes the image as stored in the bitmap file

Image::Image(const char* fmt_filename, ...)
{
    if (!fmt_filename || !*fmt_filename)
        throw "Could not create image from file";

    va_list ap;

    // Unwrap the format
    char filename[512];

    va_start(ap, fmt_filename);
    if (vsnprintf(filename, 512, fmt_filename, ap) < 0) 
        throw "Could not create image from file";
    va_end(ap);

    if(!load(filename)) 
        throw "Could not create image from file";
}

// Copies the other image

Image::Image(const Image& other)
	:width_(other.width_), height_(other.height_)
{
	pixels_ = (Color*)calloc(width_ * height_, sizeof(Color));
	memcpy(pixels_, other.pixels_, width_ * height_ * sizeof(Color));
}

// Copies the other image

Image& Image::operator=(const Image& other)
{
    if (&other == this) 
        return *this;

    if (pixels_)
        free(pixels_);

    width_ = other.width_;
    height_ = other.height_;
    pixels_ = (Color*)calloc(width_ * height_, sizeof(Color));
    memcpy(pixels_, other.pixels_, width_ * height_ * sizeof(Color));

    return *this;
}

// Stores a copy of the color pointer

Image::Image(Color* pixels, unsigned int width, unsigned int height)
    :width_{ width }, height_{ height }, pixels_{ (Color*)calloc(width * height, sizeof(Color)) }
{
    memcpy(pixels_, pixels, width * height * sizeof(Color));
}

// Creates an image with the specified size and color

Image::Image(unsigned int w, unsigned int h, Color color)
{
    width_ = w;
    height_ = h;

    pixels_ = (Color*)calloc(width_ * height_, sizeof(Color));
    
    if (color != Color::Transparent)
    {
        for (unsigned int i = 0; i < width_ * height_; i++)
            pixels_[i] = color;
    }
}

// Frees the pixel pointer

Image::~Image()
{
	free(pixels_);
}

/*
-------------------------------------------------------------------------------------------------------
User end functions
-------------------------------------------------------------------------------------------------------
*/

// Returns the pointer to the image pixels as a color array

Color* Image::pixels()
{
    return pixels_;
}

// Returns the image width

unsigned Image::width() const
{
    return width_;
}

// Returns the image height

unsigned Image::height() const
{
    return height_;
}

// Returns a color reference to the specified pixel coordinates

Color& Image::operator()(unsigned int row, unsigned int col)
{
    return pixels_[row * width_ + col];
}

// Returns a constant color reference to the specified pixel coordinates

const Color& Image::operator()(unsigned int row, unsigned int col) const
{
    return pixels_[row * width_ + col];
}

/*
-------------------------------------------------------------------------------------------------------
BMP image files functions
-------------------------------------------------------------------------------------------------------
*/

// Loads an image from the specified file path

bool Image::save(const char* fmt_filename, ...) const
{
    if (!fmt_filename || !*fmt_filename)
        return 0;

    va_list ap;

    // Unwrap the format
    char filename[512];

    va_start(ap, fmt_filename);
    if (vsnprintf(filename, 512, fmt_filename, ap) < 0) return false;
    va_end(ap);

    unsigned c = 0;
    while (filename[++c] && filename[c] != '.');
    filename[c++] = '.';
    filename[c++] = 'b';
    filename[c++] = 'm';
    filename[c++] = 'p';
    filename[c++] = '\0';

    FILE* file = fopen(filename, "wb");
    if (!file) 
        return false;

    const uint16_t bpp = 32u;
    const uint32_t bytes_per_pixel = 4u;

    // BMP rows are aligned to 4 bytes. For 32bpp stride == width*4 already aligned.
    const uint32_t row_stride = width_ * bytes_per_pixel;
    const uint32_t pixel_data_size = row_stride * height_;

    // --- FILE HEADER (14 bytes) ---
    // Signature "BM"
    if (fwrite("BM", sizeof(uint8_t), 2, file) != 2)
    {
        fclose(file);
        return false;
    }

    const uint32_t bfOffBits = 14u + 40u; // file header + BITMAPINFOHEADER
    const uint32_t bfSize = bfOffBits + pixel_data_size;
    write_le32(file, bfSize);
    write_le16(file, 0); // bfReserved1
    write_le16(file, 0); // bfReserved2
    write_le32(file, bfOffBits);

    // --- DIB HEADER: BITMAPINFOHEADER (40 bytes) ---
    write_le32(file, 40u);                 // biSize
    write_le32(file, (uint32_t)width_);    // biWidth
    write_le32(file, (uint32_t)height_);   // biHeight (positive => bottom-up)
    write_le16(file, 1u);                  // biPlanes
    write_le16(file, bpp);                 // biBitCount
    write_le32(file, 0u);                  // biCompression (BI_RGB = 0)
    write_le32(file, pixel_data_size);     // biSizeImage
    write_le32(file, 2835u);               // biXPelsPerMeter (~72 DPI)
    write_le32(file, 2835u);               // biYPelsPerMeter
    write_le32(file, 0u);                  // biClrUsed
    write_le32(file, 0u);                  // biClrImportant

    // --- Pixel data (bottom-up BGR[A]) ---

    uint8_t* row = (uint8_t*)calloc(row_stride, sizeof(uint8_t));
    for (int y = (int)height_ - 1; y >= 0; --y)
    {
        uint8_t* out = row;
        const Color* in = &pixels_[(size_t)y * width_];

            // BGRA
        for (unsigned x = 0; x < width_; ++x)
        {
            *out++ = in[x].B;
            *out++ = in[x].G;
            *out++ = in[x].R;
            *out++ = in[x].A;
        }

        if (fwrite(row, sizeof(uint8_t), row_stride, file) != row_stride)
        {
            free(row);
            fclose(file);
            return false;
        }
    }

    free(row);
    fclose(file);
    return true;
}

// Saves the image to the specified file path

bool Image::load(const char* fmt_filename, ...) 
{
    if (!fmt_filename || !*fmt_filename)
        return 0;

    va_list ap;

    // Unwrap the format
    char filename[512];

    va_start(ap, fmt_filename);
    if (vsnprintf(filename, 512, fmt_filename, ap) < 0) return false;
    va_end(ap);

    unsigned c = 0;
    while (filename[++c] && filename[c] != '.');
    filename[c++] = '.';
    filename[c++] = 'b';
    filename[c++] = 'm';
    filename[c++] = 'p';
    filename[c++] = '\0';

    FILE* file = fopen(filename, "rb");
    if (!file) return false;

    // Check signature
    char sig[2];
    if (fread(sig, 1, 2, file) != 2 || sig[0] != 'B' || sig[1] != 'M')
    {
        fclose(file);
        return false;
    }

    (void)read_le32(file); // bfSize
    (void)read_le16(file); // bfReserved1
    (void)read_le16(file); // bfReserved2
    uint32_t bfOffBits = read_le32(file);

    uint32_t biSize = read_le32(file);
    if (biSize < 40u) 
    { 
        fclose(file); 
        return false; 
    } // we expect BITMAPINFOHEADER or larger

    int32_t biWidth = (int32_t)read_le32(file);
    int32_t biHeight = (int32_t)read_le32(file);
    uint16_t biPlanes = read_le16(file);
    uint16_t biBitCount = read_le16(file);
    uint32_t biCompression = read_le32(file);
    (void)read_le32(file); // biSizeImage
    (void)read_le32(file); // biXPelsPerMeter
    (void)read_le32(file); // biYPelsPerMeter
    (void)read_le32(file); // biClrUsed
    (void)read_le32(file); // biClrImportant

    if (biPlanes != 1 || (biBitCount != 24 && biBitCount != 32) || (biCompression != 0 && biCompression != 3))
    {
        fclose(file);
        return false; // only uncompressed 24/32-bit
    }

    // Some BMPs have extra header bytes; seek to pixel array
    if (fseek(file, (long)bfOffBits, SEEK_SET) != 0) 
    { 
        fclose(file); 
        return false; 
    }

    const bool top_down = (biHeight < 0);
    const uint32_t w = (uint32_t)biWidth;
    const uint32_t h = (uint32_t)(top_down ? -biHeight : biHeight);
    const uint32_t bytes_per_pixel = (biBitCount == 32) ? 4u : 3u;
    const uint32_t row_stride = ((w * bytes_per_pixel + 3u) / 4u) * 4u;

    // allocate
    if (pixels_)
        free(pixels_);

    width_ = w; height_ = h;
    pixels_ = (Color*)calloc((size_t)w * h, sizeof(Color));

    uint8_t* row = (uint8_t*)calloc(row_stride, sizeof(uint8_t));
    for (uint32_t y = 0; y < h; ++y)
    {
        uint8_t* in = row;
        if (fread(in, sizeof(uint8_t), row_stride, file) != row_stride) 
        { 
            free(row); 
            fclose(file); 
            return false; 
        }

        uint32_t dst_y = top_down ? y : (h - 1 - y);
        Color* out = &pixels_[(size_t)dst_y * w];

        if (biBitCount == 32)
            for (uint32_t x = 0; x < w; ++x)
            {
                out[x].B = *(in++);
                out[x].G = *(in++);
                out[x].R = *(in++);
                out[x].A = *(in++);
            }

        else
            for (uint32_t x = 0; x < w; ++x)
            {
                out[x].B = *(in++);
                out[x].G = *(in++);
                out[x].R = *(in++);
                out[x].A = 255;
            }
    }

    free(row);
    fclose(file);
    return true;
}


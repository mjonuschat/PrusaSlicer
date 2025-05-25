#include "Slic3r/App/Render/ImageCodec.hpp"

#include <Slic3r/Log.hpp>
#include <Slic3r/Memory.hpp>

#include <sstream>
#include <bit>
#include <boost/algorithm/string/predicate.hpp>

#include <png.h>
#include <nanosvg/nanosvg.h>
//#define NANOSVGRAST_IMPLEMENTATION
#include <nanosvg/nanosvgrast.h>

namespace Slic3r::App::Render {

void flip_pixels_in_y(Image::Data& pixels, size_t h, size_t row_stride)
{
    Image::Data temp_row(row_stride, 0);
    const auto h_half = h / 2;
    for (size_t row = 0; row < h_half; row++) {
        std::memcpy(temp_row.data(), &pixels[row * row_stride], row_stride);
        const auto opposite_row = h - row - 1;
        std::memcpy(&pixels[row * row_stride], &pixels[opposite_row * row_stride], row_stride);
        std::memcpy(&pixels[opposite_row * row_stride], temp_row.data(), row_stride);
    }
}

class PngReadCodec : public IImageLoadCodec
{
public:
    ~PngReadCodec() override;

    std::vector<Image> load(std::istream& is, const ImageLoadOptions& opts, Size* image_size = nullptr) override;
    bool matches(const std::string& filename) override;

private:
    static void read_callback(
        png_struct* png_ptr, png_bytep out_bytes, png_size_t byte_count_to_read
    );

private:
    png_struct* png{nullptr};
    png_info* info{nullptr};
};

class SvgReadCodec : public IImageLoadCodec
{
public:
    bool matches(const std::string& filename) override;
    std::vector<Image> load(std::istream& is, const ImageLoadOptions& opts, Size* image_size = nullptr) override;

private:
    static NSVGimage* load_svg(std::istream& input);
};

PngReadCodec::~PngReadCodec()
{
    if (png && info)
        png_destroy_info_struct(png, &info);
    if (png)
        png_destroy_read_struct(&png, nullptr, nullptr);
}

std::vector<Image> PngReadCodec::load(std::istream& is, const ImageLoadOptions& opts, Size* image_size)
{
    static const constexpr int PNG_SIG_BYTES = 8;
    std::vector<Image> ret;

    std::vector<png_byte> sig(PNG_SIG_BYTES, 0);
    is.read(reinterpret_cast<char*>(sig.data()), PNG_SIG_BYTES);
    if (!png_check_sig(sig.data(), PNG_SIG_BYTES)) {
        SPDLOG_ERROR("Failed parsing PNG header");
        return ret;
    }

    png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);

    if (!png) {
        SPDLOG_ERROR("Failed parsing PNG data");
        return ret;
    }

    info = png_create_info_struct(png);
    if (!info) {
        SPDLOG_ERROR("Failed parsing PNG data");
        return ret;
    }

    png_set_read_fn(png, static_cast<void*>(&is), read_callback);

    // Tell that we have already read the first bytes to check the signature
    png_set_sig_bytes(png, PNG_SIG_BYTES);

    png_read_info(png, info);

    int image_width = png_get_image_width(png, info);
    int image_height = png_get_image_height(png, info);
    size_t color_type = png_get_color_type(png, info);
    size_t bit_depth = png_get_bit_depth(png, info);
    size_t channels = png_get_channels(png, info);
    size_t pixel_stride = bit_depth / 8 * channels;
    PixelFormat out_format;

    switch (color_type) {
    case PNG_COLOR_TYPE_RGB:
        out_format = PixelFormat::RGB8;
        break;

    case PNG_COLOR_TYPE_RGBA:
        out_format = PixelFormat::RGBA8;
        break;

    default:
        SPDLOG_ERROR("Unsupported PNG format with color type: {}", color_type);
        return ret;
    }

    if (bit_depth != 8) {
        SPDLOG_ERROR("Unsupported PNG bit depth: {}", bit_depth);
        return ret;
    }

    const Size out_size{image_width, image_height};
    // Store PNG real size
    if (image_size) {
        *image_size = out_size;
    }

    Image::Data out_pixels(image_width * image_height * pixel_stride, 0);

    auto readbuf = static_cast<png_bytep>(out_pixels.data());
    for (size_t r = 0; r < image_height; ++r) {
        size_t r_idx = opts.flip_y ? image_height - r - 1 : r;
        png_read_row(png, readbuf + r_idx * image_width * pixel_stride, nullptr);
    }

    const Size resolved_size = opts.resolve_to_size(out_size);

    Image img(out_format, image_width, image_height, std::move(out_pixels));
    if (resolved_size != out_size) {
        ret.emplace_back(img.rescaled_with_preserved_ratio(resolved_size));
    }
    else
        ret.emplace_back(std::move(img));

    if (opts.gen_mipmaps) {
        while (ret.back().width() > 1 || ret.back().height() > 1) {
            ret.emplace_back(ret.back().half_sampled());
        }
    }

    return ret;
}

void PngReadCodec::read_callback(
    png_struct* png_ptr, png_bytep out_bytes, png_size_t byte_count_to_read
)
{
    // Retrieve our input buffer through the png_ptr
    auto reader = static_cast<std::istream*>(png_get_io_ptr(png_ptr));

    if (!reader || !reader->good())
        return;

    reader->read(reinterpret_cast<char*>(out_bytes), byte_count_to_read);
}
bool PngReadCodec::matches(const std::string& filename)
{
    return boost::algorithm::iends_with(filename, ".png");
}

std::vector<Image> SvgReadCodec::load(std::istream& is, const ImageLoadOptions& opts, Size* image_size)
{
    std::vector<Image> ret;
    FuncDeleter<NSVGimage> d{nsvgDelete};
    std::unique_ptr<NSVGimage, FuncDeleter<NSVGimage>> image{load_svg(is), d};

    if (!image) {
        SPDLOG_ERROR("Failed parsing SVG file");
        return ret;
    }

    Size size = opts.resolve_to_size({static_cast<int>(image->width), static_cast<int>(image->height)});

    float scale_w = static_cast<float>(size.width) / image->width;
    float scale_h = static_cast<float>(size.height) / image->height;

    NSVGrasterizer* rast = nsvgCreateRasterizer();
    if (!rast) {
        SPDLOG_ERROR("Creating SVG rasterizer failed");
        return ret;
    }

    // Store SVG size, not the scaled one
    if (image_size) {
        image_size->width = image->width;
        image_size->height = image->height;
    }

    do {
        // std::vector<unsigned char> data(n_pixels * 4, 0);
        Image::Data pixels(size.space() * 4, 0);
        nsvgRasterizeXY(
            rast, image.get(), 0, 0, scale_w, scale_h, pixels.data(), size.width, size.height, size.width * 4
        );

        if (opts.flip_y)
            flip_pixels_in_y(pixels, size.height, size.width * 4);

        if (std::any_of(pixels.begin(), pixels.end(), [](auto x){ return x!=0; })) {
            SPDLOG_INFO("Non empty image with size {}x{} added", size.width, size.height);
        } else {
            SPDLOG_INFO("Empty image with size {}x{} added", size.width, size.height);
        }
        ret.emplace_back(PixelFormat::RGBA8, size.width, size.height, std::move(pixels));

        if (opts.gen_mipmaps) {
            if (size.width == 1 && size.height == 1)
                break;

            if (size.width > 1) size.width /= 2;
            if (size.height > 1) size.height /= 2;

            scale_w = (float) size.width / image->width;
            scale_h = (float) size.height / image->height;
        }
    } while (opts.gen_mipmaps);

    return ret;
}

NSVGimage* SvgReadCodec::load_svg(std::istream& input)
{
    const char* units = "px";
    const float dpi = 96;

    std::stringstream buffer;
    buffer << input.rdbuf();
    NSVGimage* image = nsvgParse(buffer.str().data(), units, dpi);
    return image;
}

bool SvgReadCodec::matches(const std::string& filename)
{
    return boost::algorithm::iends_with(filename, ".svg");
}

ImageCodecManager::ImageCodecManager()
{
    // register known codecs
    m_loaders.emplace_back(new PngReadCodec());
    m_loaders.emplace_back(new SvgReadCodec());
}

ImageCodecManager& ImageCodecManager::instance()
{
    static ImageCodecManager inst;
    return inst;
}

IImageLoadCodec* ImageCodecManager::find_loader(const std::string& filename)
{
    for (const auto& reader : m_loaders) {
        if (reader->matches(filename)) {
            return reader.get();
        }
    }

    return nullptr;
}

Size ImageLoadOptions::resolve_to_size(const Size &source_size) const
{
    Size resolved_size = source_size.scaled(Size{max_size_px, max_size_px}, Size::ScaleMode::KeepAspectRatio);

    if (force_power_of_two) {
        int size = std::bit_ceil(static_cast<uint32_t>(std::max(resolved_size.width, resolved_size.height)));
        resolved_size.width = size;
        resolved_size.height = size;
    }

    return resolved_size;
}

} // namespace Slic3r::App::Render

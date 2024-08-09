#include "ImageCodec.hpp"

#include <Slic3r/Log.hpp>
#include <Slic3r/Memory.hpp>

#include <sstream>
#include <boost/core/bit.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <png.h>
#include <nanosvg/nanosvg.h>
#define NANOSVGRAST_IMPLEMENTATION
#include <nanosvg/nanosvgrast.h>

namespace Slic3r::App::Render {

void adjust_size_to_opts(size_t& w, size_t& h, const ImageLoadOptions& opts)
{
    w = std::min<size_t>(w, opts.max_size_px);
    h = std::min<size_t>(h, opts.max_size_px);
    if (opts.force_power_of_two)
        w = h = boost::core::bit_ceil(std::max(w, h));
}

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

    std::vector<Image> load(std::istream& is, const ImageLoadOptions& opts) override;
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
    std::vector<Image> load(std::istream& is, const ImageLoadOptions& opts) override;

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

std::vector<Image> PngReadCodec::load(std::istream& is, const ImageLoadOptions& opts)
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

    size_t out_w = png_get_image_width(png, info);
    size_t out_h = png_get_image_height(png, info);
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

    Image::Data out_pixels(out_w * out_h * pixel_stride, 0);

    auto readbuf = static_cast<png_bytep>(out_pixels.data());
    for (size_t r = 0; r < out_h; ++r) {
        size_t r_idx = opts.flip_y ? out_h - r - 1 : r;
        png_read_row(png, readbuf + r_idx * out_w * pixel_stride, nullptr);
    }

    size_t new_w = out_w;
    size_t new_h = out_h;
    adjust_size_to_opts(new_w, new_h, opts);

    if (new_w != out_w || new_h != out_h) {
        // TODO: resize dest image

    }

    ret.emplace_back(out_format, out_w, out_h, std::move(out_pixels));

    if (opts.gen_mipmaps) {
        // generate mipmaps
        while (ret.back().width() > 1) {
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

std::vector<Image> SvgReadCodec::load(std::istream& is, const ImageLoadOptions& opts)
{
    std::vector<Image> ret;
    FuncDeleter<NSVGimage> d{nsvgDelete};
    std::unique_ptr<NSVGimage, FuncDeleter<NSVGimage>> image{load_svg(is), d};

    if (!image) {
        SPDLOG_ERROR("Failed parsing SVG file");
        return ret;
    }

    const float scale = opts.max_size_px / std::max<float>(image->width, image->height);

    size_t width = scale * image->width;
    size_t height = scale * image->height;

    float scale_w = (float) width / image->width;
    float scale_h = (float) height / image->height;

    PixelFormat format = PixelFormat::RGBA8;
    width = (int) (scale * image->width);
    height = (int) (scale * image->height);

    adjust_size_to_opts(width, height, opts);

    NSVGrasterizer* rast = nsvgCreateRasterizer();
    if (!rast) {
        SPDLOG_ERROR("Creating SVG rasterizer failed");
        return ret;
    }

    do {
        // std::vector<unsigned char> data(n_pixels * 4, 0);
        Image::Data pixels(width * height * 4, 0);
        nsvgRasterizeXY(
            rast, image.get(), 0, 0, scale_w, scale_h, pixels.data(), width, height, width * 4
        );

        if (opts.flip_y)
            flip_pixels_in_y(pixels, height, width * 4);

        ret.emplace_back(format, width, height, std::move(pixels));

        width /= 2;
        height /= 2;
    } while (opts.gen_mipmaps && width > 1);

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
    for (const auto& reader : m_loaders)
        if (reader->matches(filename))
            return reader.get();

    return nullptr;
}

} // namespace Slic3r::App::Render

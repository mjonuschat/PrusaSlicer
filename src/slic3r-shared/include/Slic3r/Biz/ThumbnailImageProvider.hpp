#pragma once

#include "libslic3r/IThumbnailImageGenerator.hpp"

namespace Slic3r::Biz {

class ThumbnailImageProvider
{
public:
    std::future<Slicing::ThumbnailImageResults> generate_thumbnails(
        const Slicing::ThumbnailImageRequests& requests
    );

    void set_generator(Slicing::IThumbnailImageGenerator& generator)
    {
        m_generator = &generator;
    }

private:
    Slicing::IThumbnailImageGenerator* m_generator{nullptr};
};

} // namespace Slic3r::Biz

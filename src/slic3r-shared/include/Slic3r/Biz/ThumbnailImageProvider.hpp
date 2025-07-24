#pragma once

#include "Slic3r/Biz/IThumbnailImageGenerator.hpp"

namespace Slic3r::Biz {

class ThumbnailImageProvider
{
public:
    std::future<ThumbnailImageResults> generate_thumbnails(const ThumbnailImageRequests& requests);

    void set_generator(IThumbnailImageGenerator& generator)
    {
        m_generator = &generator;
    }

private:
    IThumbnailImageGenerator* m_generator{nullptr};
};

} // namespace Slic3r::Biz

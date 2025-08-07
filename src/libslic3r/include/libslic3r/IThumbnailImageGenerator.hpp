#pragma once

#include "libslic3r/ThumbnailImageRequest.hpp"
#include "libslic3r/ThumbnailImageResult.hpp"

#include <future>

namespace Slic3r::Biz::Slicing {

class IThumbnailImageGenerator
{
public:
    virtual ~IThumbnailImageGenerator() = default;

    virtual void enqueue_thumbnail_requests(
        const ThumbnailImageRequests& requests,
        std::promise<ThumbnailImageResults>&& promise
    )                                       = 0;
    virtual void handle_enqueued_requests() = 0;
};

} // namespace Slic3r::Biz

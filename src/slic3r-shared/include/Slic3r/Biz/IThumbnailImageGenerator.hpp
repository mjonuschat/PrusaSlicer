#pragma once

#include "Slic3r/Biz/ThumbnailImageRequest.hpp"
#include "Slic3r/Biz/ThumbnailImageResult.hpp"

#include <future>

namespace Slic3r::Biz {

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

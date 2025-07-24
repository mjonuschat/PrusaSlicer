#include "Slic3r/Biz/ThumbnailImageProvider.hpp"
#include "Slic3r/Biz/Platform/PlatformServices.hpp"

#include "Slic3r/Log.hpp"

namespace Slic3r::Biz {

std::future<ThumbnailImageResults> ThumbnailImageProvider::generate_thumbnails(
    const ThumbnailImageRequests& requests
)
{
    auto& main_thread_dispatcher = Platform::PlatformServices::instance().main_thread_dispatcher();

    std::promise<ThumbnailImageResults> promise;
    std::future<ThumbnailImageResults> ret = promise.get_future();

    if (m_generator == nullptr)
        promise.set_value(ThumbnailImageResults());
    else {
        if (!main_thread_dispatcher.dispatch_on_main_thread(
                [this, r = std::move(requests), p = std::move(promise)]() mutable {
            m_generator->enqueue_thumbnail_requests(r, std::move(p));
        }
            ))
        {
            SPDLOG_INFO("Thumbnail image request not dispatched");
        }
    }
    return ret;
}

} // namespace Slic3r::Biz

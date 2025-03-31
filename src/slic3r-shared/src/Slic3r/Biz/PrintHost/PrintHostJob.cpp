#include "Slic3r/Biz/PrintHost/PrintHostJob.hpp"
#include "Slic3r/Biz/PrintHost/PrintHostFactory.hpp"

#include "Slic3r/Log.hpp"


namespace Slic3r::Biz::PrintHost {

PrintHostJob::PrintHostJob(IPrintHostJobCallbacks* owner, size_t id, PrintHostConfig config, PrintHostJobData data)
    : m_owner(owner)
    , m_id(id)
    , m_print_host(create_print_host(std::move(config)))
    , m_upload_data(std::move(data))
    , m_host(m_print_host->get_host())
    , m_future(m_promise.get_future().share())
{
}

PrintHostJob::~PrintHostJob() = default;

void PrintHostJob::cancel()
{
    if (m_thread.joinable())
    {
        m_thread.request_stop();
    }
}

void PrintHostJob::start()
{
    if (m_thread.joinable())
    {
        assert(false);
        return;
    }

    m_thread = JThread::JThread([&](JThread::StopToken stop_token) {
        for (const auto& dep : m_dependencies) {
            dep->get_future().wait();  // Wait for dependency to finish
        }
        m_owner->on_job_progress(m_id, 0); // Indicate the upload is starting

        bool success = m_print_host->perform(std::move(m_upload_data),
            [this, stop_token](Network::IHttp::Progress progress, bool &cancel) 
            { 
                this->on_progress_fn(std::move(progress), cancel); 
                if(stop_token.stop_requested()) 
                    cancel = true; 
            },
            [this, stop_token](Network::IHttp::Retry retry, bool &cancel) 
            { 
                this->on_retry_fn(std::move(retry), cancel); 
                if(stop_token.stop_requested()) 
                    cancel = true; 
            },
            [this](std::string error)                            { this->on_error_fn(std::move(error)); },
            [this](std::string tag, std::string host)            { this->on_info_fn(std::move(tag), std::move(host)); }
           
        );
        m_promise.set_value();

        if (success) {
             m_owner->on_job_progress(m_id, 100);
        }
        m_owner->on_job_done(m_id);
    });
}

void PrintHostJob::on_progress_fn(Network::IHttp::Progress&& progress, bool &cancel)
{
    SPDLOG_INFO("PrintHostJob::on_progress_fn id:{} progress: {}", std::to_string(m_id), progress.to_string());
    int prg = (progress.ultotal > 0 ? 100*progress.ulnow / progress.ultotal : 0);
    m_owner->on_job_progress(m_id, prg);
}
void PrintHostJob::on_retry_fn(Network::IHttp::Retry&& retry, bool&cancel)
{
    SPDLOG_INFO("PrintHostJob::on_retry_fn id:{} retry: {}", std::to_string(m_id), retry.to_string());
}
void PrintHostJob::on_error_fn(std::string&& error)
{
    m_owner->on_job_error(m_id, error);

}
void PrintHostJob::on_info_fn(std::string&& tag, std::string&& host)
{
    m_owner->on_job_info(m_id, tag, host);
}

std::shared_future<void> PrintHostJob::get_future() 
{
    return m_future;
}
void PrintHostJob::add_dependency(std::shared_ptr<PrintHostJob> dependency) 
{
    m_dependencies.push_back(dependency);
}

}
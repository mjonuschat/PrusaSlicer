#include "Slic3r/Biz/PrintHost/PrintHostJobManager.hpp"

#include "Slic3r/Biz/PrintHost/PrintHostJob.hpp"
#include "Slic3r/Log.hpp"
#include "Slic3r/IdGenerator.hpp"

#include "libslic3r/format.hpp"

namespace Slic3r::Biz::PrintHost {

namespace {
static size_t next_id() {
    static IdGenerator<size_t> id_generator(0);
    return id_generator.next_id();
}
} // namespace

PrintHostJobManager::PrintHostJobManager(Platform::IMainThreadDispatcher& dispatcher) 
    : m_dispatcher{dispatcher}
    , m_done_listener{std::make_unique<PrintHostDoneListener>(std::bind(&PrintHostJobManager::erase_job, this, std::placeholders::_1))}
{
    add_listener<IPrintHostListener>(m_done_listener.get());
}

PrintHostJobManager::~PrintHostJobManager() 
{
    ASSERT(
        m_dispatcher.is_closed(),
        "There must be no queued events (not even in the future),"
        " because they may remember the address of this instance!"
    );
    
}

size_t PrintHostJobManager::emplace_job(PrintHostConfig config, PrintHostJobData data)
{
    size_t id = next_id();
    m_job_map[id] = std::make_shared<PrintHostJob>(this, id, std::move(config), std::move(data));
    for (const auto& [key, value] : m_job_map) {
        if (key == id) {
            continue;
        }
        if (value->get_host() ==  m_job_map[id]->get_host()) {
            ASSERT(key < id, "We expect id to be the latest value.");
            m_job_map[id]->add_dependency(value);   
        }
    }
    m_job_map[id]->start();
    return id;
}
void PrintHostJobManager::cancel(size_t id)
{
    m_job_map[id]->cancel();
}

void PrintHostJobManager::on_job_progress(size_t id, int progress)
{
    {
        std::lock_guard<std::mutex> lock(m_dispatcher_mutex);
        
        bool dispatched = m_dispatcher.dispatch_on_main_thread([this, id, progress](){
            this->invoke_listeners<IPrintHostListener>([id, progress](auto* listener){
                listener->on_print_host_progress(id, progress);
            });
        });
    }

}
void PrintHostJobManager::on_job_error(size_t id, const std::string& error)
{
    {
        std::lock_guard<std::mutex> lock(m_dispatcher_mutex);
        bool dispatched = m_dispatcher.dispatch_on_main_thread([this, id, error](){
            this->invoke_listeners<IPrintHostListener>([id, error](auto* listener){
                listener->on_print_host_error(id, error);
            });
        });
    }
}
void PrintHostJobManager::on_job_cancel(size_t id)
{
    {
        std::lock_guard<std::mutex> lock(m_dispatcher_mutex);
        bool dispatched = m_dispatcher.dispatch_on_main_thread([this, id](){
            this->invoke_listeners<IPrintHostListener>([id](auto* listener){
                listener->on_print_host_cancel(id);
            });
        });
    }
}
void PrintHostJobManager::on_job_done(size_t id)
{
    {
        std::lock_guard<std::mutex> lock(m_dispatcher_mutex);
        bool dispatched = m_dispatcher.dispatch_on_main_thread([this, id](){
            this->invoke_listeners<IPrintHostListener>([id](auto* listener){
                listener->on_print_host_done(id);
            });
        });
    }
}

void PrintHostJobManager::on_job_info(size_t id, const std::string& tag, const std::string& msg)
{
    {
        std::lock_guard<std::mutex> lock(m_dispatcher_mutex);
        bool dispatched = m_dispatcher.dispatch_on_main_thread([this, id, tag, msg](){
            this->invoke_listeners<IPrintHostListener>([id, tag, msg](auto* listener){
                listener->on_print_host_info(id, tag, msg);
            });
        });
    }
}



void PrintHostJobManager::erase_job(size_t id)
{
    m_job_map.erase(id);
}


}
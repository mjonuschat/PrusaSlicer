#include "Slic3r/Biz/AppInstance/AbstractAppInstanceMessageHandler.hpp"

#include <Slic3r/Biz/Platform/PlatformServices.hpp>
#include "Slic3r/Biz/Platform/ISingleInstanceChecker.hpp"
#include "Slic3r/Log.hpp"
#include "Slic3r/Assert.hpp"
#include "Slic3r/Biz/Algorithms/StringUtils.hpp"


#include <nlohmann/json.hpp>
#include <boost/filesystem.hpp>
#include <vector>

namespace fs = boost::filesystem;

namespace Slic3r::Biz::AppInstance {

namespace {
fs::path get_path(const std::string& possible_path) 
{
    if (possible_path.empty() || possible_path.size() < 3) {
        return {};
    }
    fs::path p(possible_path);
    if (fs::exists(p)) {
        return p;
    }
    if (possible_path.front() == '\"' && possible_path.back() == '\"' && possible_path.size() > 2) {
        fs::path quoted_p(possible_path.substr(1, possible_path.size() - 2));
        if (fs::exists(quoted_p)) {
            return quoted_p;
        }
    }
    return {};
}
}

AbstractAppInstanceMessageHandler::AbstractAppInstanceMessageHandler(Platform::IMainThreadDispatcher& dispatcher)
    : m_dispatcher{dispatcher}
{
     m_message_handlers["CLI"] = std::bind(&AbstractAppInstanceMessageHandler::handle_message_type_cli, this, std::placeholders::_1);
     m_message_handlers["STORE_READ"] = std::bind(&AbstractAppInstanceMessageHandler::handle_message_type_store_read, this, std::placeholders::_1);
     m_message_handlers["OTHER_CLOSING"] = std::bind(&AbstractAppInstanceMessageHandler::handle_message_type_other_closed, this, std::placeholders::_1);
}

AbstractAppInstanceMessageHandler::~AbstractAppInstanceMessageHandler()
{
    SPDLOG_INFO(__FUNCTION__);
    ASSERT(
        m_dispatcher.is_closed(),
        "There must be no queued events (not even in the future),"
        " because they may remember the address of this instance!"
    );
}

void AbstractAppInstanceMessageHandler::handle_message(const std::string& message) 
{
    ASSERT(this != nullptr);
    SPDLOG_INFO("Message from another instance {}", message);
    // message in format { "type" : "TYPE", "data" : "data" }
    // types: CLI, STORE_READ
    std::string type;
    std::string data;
    
    try {
        nlohmann::json j = nlohmann::json::parse(message);
        if (j.contains("type")) {
            type = j["type"].get<std::string>();
        }
        if (j.contains("data")) {
            data = j["data"].get<std::string>();
        }
    } catch (const nlohmann::json::exception& e) {
        SPDLOG_ERROR("Could not parse other instance message: {}", e.what());
        return;
    }

    ASSERT(!type.empty());
    ASSERT(m_message_handlers.find(type) != m_message_handlers.end(), "Message type that has no handler callback set."); 
    if (m_message_handlers.find(type) != m_message_handlers.end()) {
        m_message_handlers[type](data);
    }
}

void AbstractAppInstanceMessageHandler::handle_message_type_cli(const std::string& data)
{
    SPDLOG_INFO(__FUNCTION__);
    if (!Biz::Platform::PlatformServices::instance().single_instance_checker().is_primary_instance())
    {
        SPDLOG_INFO("Handling message from another app instance while not being primary app instance. Aborting.");
        return;
    }
    std::vector<std::string> args;
	bool parsed = Algorithms::unescape_strings_cstyle(data, args);
	assert(parsed);
	if (! parsed) {
		SPDLOG_ERROR("message from other instance is incorrectly formatted: {}", data);
		return;
	}

    
	std::vector<boost::filesystem::path> paths;
	std::vector<std::string> downloads;
	for (auto it = args.begin(); it != args.end(); ++ it) {
        SPDLOG_INFO(*it);
		boost::filesystem::path p = get_path(*it);
		if (! p.string().empty())
			paths.emplace_back(p);
		else if (it->rfind("prusaslicer://open", 0) == 0)
			downloads.emplace_back(*it);
		else if (it->rfind("prusaslicer://login", 0) == 0) {
            std::string url(*it);
            {
                std::lock_guard<std::mutex> lock(m_dispatcher_mutex);
                m_dispatcher.dispatch_on_main_thread([this,url](){
                    invoke_listeners<IAppInstanceMessageContentListener>([url](auto* listener){
                        listener->on_login_data(url);
                    });
                });
            }
		}
	}
	if (! paths.empty()) {
        {
            std::lock_guard<std::mutex> lock(m_dispatcher_mutex);
            m_dispatcher.dispatch_on_main_thread([this, paths = std::move(paths)]() mutable {
               invoke_listeners<IAppInstanceMessageContentListener>([paths = std::move(paths)](auto* listener) mutable {
                    listener->on_open_models(std::move(paths));
                });
            });
        }
	}
	if (!downloads.empty()) {
        {
            std::lock_guard<std::mutex> lock(m_dispatcher_mutex);
            m_dispatcher.dispatch_on_main_thread([this, downloads = std::move(downloads)]() mutable {
                invoke_listeners<IAppInstanceMessageContentListener>([downloads = std::move(downloads)](auto* listener) mutable {
                    listener->on_download_models(std::move(downloads));
                });
            });
        }
	}
}

void AbstractAppInstanceMessageHandler::handle_message_type_store_read(const std::string& data)
{
    {
        std::lock_guard<std::mutex> lock(m_dispatcher_mutex);
        m_dispatcher.dispatch_on_main_thread([this](){
            invoke_listeners<IAppInstanceMessageContentListener>([](auto* listener){
                listener->on_read_token_store_message();
            });
        });
    }
}

void AbstractAppInstanceMessageHandler::handle_message_type_other_closed(const std::string& data)
{
    SPDLOG_INFO(__FUNCTION__);
    // is_another_running does acquire lockfile if available
    if(!Biz::Platform::PlatformServices::instance().single_instance_checker().is_another_running()){
        on_becoming_primary_instance();
    }
}


void AbstractAppInstanceMessageHandler::dispatch_go_to_front()
{
    {
        std::lock_guard<std::mutex> lock(m_dispatcher_mutex);
        m_dispatcher.dispatch_on_main_thread([this](){
            invoke_listeners<IAppInstanceMessageContentListener>([](auto* listener){
                listener->on_app_go_front();
            });
        });
    }
}

}

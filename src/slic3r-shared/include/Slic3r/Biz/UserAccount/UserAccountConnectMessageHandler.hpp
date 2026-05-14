#pragma once

#include "Slic3r/Biz/Platform/IMainThreadDispatcher.hpp"
#include "Slic3r/Biz/Platform/WithListeners.hpp"
#include "Slic3r/Biz/UserAccount/IUserAccountListener.hpp"

#include <string>

namespace Slic3r::Biz {
class ProjectInteractor;
}
namespace Slic3r::Biz::UserAccount {
class UserAccountCommunication;

class UserAccountConnectMessageHandler : public WithListeners<IUserAccountListener>
{
public:
    UserAccountConnectMessageHandler(Platform::IMainThreadDispatcher& dispatcher);

    void handle_select_printer_message(UserAccountCommunication& communication, const std::string& message_json);

    void do_select_printer_from_connect(ProjectInteractor& project_interactor, const std::string& printer_json);

    std::string uuid_for_upload(const ProjectInteractor& project_interactor);
private:
    Platform::IMainThreadDispatcher& m_dispatcher;

    void parse_connect_printers_for_selection(const std::string& message_json, const std::string& uuid);

    std::string m_last_printer_json; 
};
}
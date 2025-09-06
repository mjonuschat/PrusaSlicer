#pragma once

namespace Slic3r::App::Launcher {

class SentryScope
{
public:
    SentryScope();
    ~SentryScope();

    void send_test_message();
};

}

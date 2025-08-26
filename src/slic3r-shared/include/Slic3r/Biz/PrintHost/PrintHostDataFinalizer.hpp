#pragma once

#include "Slic3r/Biz/PrintHost/IPrintHostBinarizeListener.hpp"
#include "Slic3r/Biz/PrintHost/PrintHostConfig.hpp"
#include "Slic3r/Biz/Platform/WithListeners.hpp"
#include "Slic3r/Biz/Platform/IMainThreadDispatcher.hpp"

#include <mutex>

namespace Slic3r::Biz::PrintHost {

class PrintHostDataFinalizer : public WithListeners<IPrintHostBinarizeListener>
{
public:
    PrintHostDataFinalizer(Platform::IMainThreadDispatcher& dispatcher);
    ~PrintHostDataFinalizer();

    /**
     * @brief In worker thread takes DataPtrVariant in PrintHostJobData, stores it in a temporary file which path is stored in PrintHostJobData::source_path.
     * On success, moves config and data further.
     */
    void finalize(PrintHostConfig&& config, PrintHostJobData&& data);
private:
    mutable std::mutex m_dispatcher_mutex;
    Platform::IMainThreadDispatcher &m_dispatcher;

    void dispatch_success(PrintHostConfig config, PrintHostJobData data);
    void dispatch_fail(const std::string& message);
};

}


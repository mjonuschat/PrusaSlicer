#pragma once

#include <cpptrace/from_current.hpp>
#include "Slic3r/Biz/Platform/IMainThreadDispatcher.hpp"
#include <jthread/JThread.hpp>
#include "Slic3r/Biz/Platform/JobManager/ProgressTracker.hpp"
#include "Slic3r/Log.hpp" // IWYU pragma: keep

namespace Slic3r::Biz::Platform::JobManager {
class JobBase
{
public:
    virtual ~JobBase()                                = default;
    virtual void cancel()                             = 0;
    virtual const ProgressTracker& progress_tracker() = 0;
};

namespace Impl {

template <typename Result>
struct get_on_result_type
{
    using type = std::function<void(Result)>;
};

template <>
struct get_on_result_type<void>
{
    using type = std::function<void()>;
};

template <typename Result>
using get_on_result_type_t = typename get_on_result_type<Result>::type;
} // namespace Impl

#ifndef NDEBUG
constexpr bool debug{true};
#else
constexpr bool debug{false};
#endif

// ArgsTuple is different to Args in that it does not contain rvalue references.
template <typename R, typename ArgsTuple, typename... Args>
class Job : public JobBase
{
public:
    using Result      = R;
    using OnResult    = Impl::get_on_result_type_t<Result>;
    using CallAfter   = std::function<void()>;
    using OnException = std::function<void(std::exception_ptr, cpptrace::stacktrace)>;

    Job& on_result(OnResult on_result)
    {
        m_on_result = on_result;
        return *this;
    }

    Job& on_exception(OnException on_exception)
    {
        m_on_exception = on_exception;
        return *this;
    }

    void start()
    {
        ASSERT(m_on_result);
        ASSERT(m_call_after);
        ASSERT(m_on_exception);

        this->m_thread = JThread::JThread{
            [this](Biz::JThread::StopToken stop_token)
            {
                const auto not_emitted{[&]() { SPDLOG_WARN("{}: job result not emitted!", m_name); }};

                m_progress_tracker.set_status(Domain::JobStatus::Started);
                CPPTRACE_TRY
                {
                    if constexpr (!std::is_void_v<Result>) {
                        auto result_in_thread{std::apply(std::move(m_function), get_args(stop_token))};
                        if (!m_dispatcher.get().dispatch_on_main_thread(
                                [on_result = m_on_result, progress_tracker = m_progress_tracker, call_after = m_call_after, result = std::move(result_in_thread)]() mutable
                                {
                                    on_result(std::move(result));
                                    progress_tracker.set_status_unsafe(Domain::JobStatus::Finished);
                                    call_after();
                                }
                            ))
                        {
                            not_emitted();
                        }
                    } else {
                        std::apply(std::move(m_function), get_args(stop_token));
                        if (!m_dispatcher.get().dispatch_on_main_thread(
                                [on_result = m_on_result, progress_tracker = m_progress_tracker, call_after = m_call_after]() mutable
                                {
                                    on_result();
                                    progress_tracker.set_status_unsafe(Domain::JobStatus::Finished);
                                    call_after();
                                }
                            ))
                        {
                            not_emitted();
                        }
                    }
                }
                CPPTRACE_CATCH(...)
                {
                    const std::exception_ptr exception{std::current_exception()};
                    const cpptrace::stacktrace stacktrace{cpptrace::from_current_exception()};
                    if (!m_dispatcher.get().dispatch_on_main_thread(
                            [on_exception = m_on_exception, progress_tracker = m_progress_tracker, call_after = m_call_after, exception, stacktrace]() mutable
                            {
                                on_exception(exception, stacktrace);
                                progress_tracker.set_status_unsafe(Domain::JobStatus::Failed);
                                call_after();
                            }
                        ))
                    {
                        SPDLOG_WARN("{}: exception thrown, but not emitted to main thread!", m_name);
                    }
                }
            }
        };
    }

private:
    friend class JobManager;

    Job(const std::string& name, std::function<R(JThread::StopToken, Args...)>&& function, IMainThreadDispatcher& dispatcher, const ProgressTracker& progress_tracker, ArgsTuple&& args) :
        m_name{name},
        m_function{std::move(function)},
        m_dispatcher{dispatcher},
        m_progress_tracker{progress_tracker},
        m_args{std::move(args)}
    {
        if constexpr (std::is_same_v<Result, void>) {
            m_on_result = []() {};
        } else {
            m_on_result = [](Result) {};
        }
    }

    std::string m_name;
    std::function<R(JThread::StopToken, Args...)> m_function;
    std::reference_wrapper<IMainThreadDispatcher> m_dispatcher;
    ProgressTracker m_progress_tracker;
    ArgsTuple m_args;
    OnResult m_on_result{};
    CallAfter m_call_after;
    OnException m_on_exception{
        [](const std::exception_ptr exception, const cpptrace::stacktrace& stacktrace)
        {
            if (debug) {
                stacktrace.print();
            }
            std::rethrow_exception(exception);
        }
    };

    // Thread is specified last to be destructed first.
    JThread::JThread m_thread;

    auto get_args(JThread::StopToken stop_token)
    {
        return std::tuple_cat(std::make_tuple(stop_token), std::move(m_args));
    }

    void call_after(CallAfter call_after)
    {
        m_call_after = call_after;
    }

    void cancel() override
    {
        m_thread.request_stop();
    }

    const ProgressTracker& progress_tracker() override
    {
        return m_progress_tracker;
    }
};
} // namespace Slic3r::Biz::Platform::JobManager

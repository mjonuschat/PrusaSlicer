///|/ Copyright (c) Prusa Research 2018 - 2023 Lukáš Matěna @lukasmatena, Vojtěch Bubník @bubnikv, Tomáš Mészáros @tamasmeszaros, Pavel Mikuš @Godrak, Roman Beránek @zavorka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef slic3r_PrintBase_hpp_
#define slic3r_PrintBase_hpp_

#include <algorithm>
#include <set>
#include <vector>
#include <string>
#include <functional>
#include <atomic>
#include <mutex>

#include <jthread/JThread.hpp>

#include "Slic3r/Domain/BedInstance.hpp"
#include "Slic3r/Domain/ConfigPack.hpp"
#include "Slic3r/Domain/ObjectID.hpp"
#include "Slic3r/Domain/Preset/SelectedPreset.hpp"
#include "Slic3r/Domain/Preset/HwConfig.hpp"
#include "Slic3r/Biz/Parser/PlaceholderParser.hpp"
#include "Slic3r/Domain/PrintStatistics.hpp"
#include "Slic3r/Domain/SlicingId.hpp"
#include "Slic3r/Domain/SLA/PrintStatistics.hpp"

#include "libslic3r/IPrint.hpp"
#include "libslic3r/SlicingStatus.hpp"
#include "libslic3r/CanceledException.hpp"
#include "libslic3r/IThumbnailImageGenerator.hpp"
#include "libslic3r/SerializedConfig.hpp"
#include "libslic3r/WipeTowerGeometry.hpp"

namespace Slic3r {
class PrintConfigView;

class PrintStateBase {
public:
    enum class State {
        // Fresh state, either the object is new or the data of that particular milestone was cleaned up.
        // Fresh state may transit to Started.
        Fresh,
        // Milestone was started and now it is being executed.
        // Started state may transit to Canceled with invalid data or Done with valid data.
        Started,
        // Milestone was being executed, but now it is canceled and not yet cleaned up.
        // Canceled state may transit to Fresh state if its invalid data is cleaned up
        // or to Started state.
        // Canceled and Invalidated states are of similar nature: Canceled step was Started but canceled,
        // while Invalidated state was Done but invalidated.
        Canceled,
        // Milestone was finished successfully, it's data is now valid.
        // Done state may transit to Invalidated state if its data is no more valid
        // or to a Started state.
        Done,
        // Milestone was finished successfully (done), but now it is invalidated and it's data is no more valid.
        // Invalidated state may transit to Fresh if its invalid data is cleaned up,
        // or to state Started.
        // Canceled and Invalidated states are of similar nature: Canceled step was Started but canceled,
        // while Invalidated state was Done but invalidated.
        Invalidated,
    };

    typedef size_t TimeStamp;

    // A new unique timestamp is being assigned to the step every time the step changes its state.
    struct StateWithTimeStamp
    {
        State       state { State::Fresh };
        TimeStamp   timestamp { 0 };
        bool        enabled { true };

        bool        is_done() const { return state == State::Done; }
        // The milestone may have some data available, but it is no more valid and it should be cleaned up to conserve memory.
        bool        is_dirty() const { return state == State::Canceled || state == State::Invalidated; }

        // If the milestone is Started or Done, invalidate it:
        // Turn Started to Canceled, turn Done to Invalidated.
        // Update timestamp of this milestone.
        bool        try_invalidate() {
            bool invalidated = this->state == State::Started || this->state == State::Done;
            if (invalidated) {
                this->state = this->state == State::Started ? State::Canceled : State::Invalidated;
                this->timestamp = ++ g_last_timestamp;
            }
            return invalidated;
        }
    };

protected:
    //FIXME last timestamp is shared between Print & SLAPrint,
    // and if multiple Print or SLAPrint instances are executed in parallel, modification of g_last_timestamp
    // is not synchronized!
    static size_t g_last_timestamp;
};

// To be instantiated over FDMPrintStep or FDMPrintObjectStep enums.
template <class StepType, size_t COUNT>
class PrintState : public PrintStateBase
{
public:
    PrintState() {}

    StateWithTimeStamp state_with_timestamp(StepType step, std::mutex &mtx) const {
        std::scoped_lock<std::mutex> lock(mtx);
        StateWithTimeStamp state = m_state[step];
        return state;
    }

    bool is_started(StepType step, std::mutex &mtx) const {
        return this->state_with_timestamp(step, mtx).state == State::Started;
    }

    bool is_done(StepType step, std::mutex &mtx) const {
        return this->state_with_timestamp(step, mtx).state == State::Done;
    }

    StateWithTimeStamp state_with_timestamp_unguarded(StepType step) const { 
        return m_state[step];
    }

    bool is_started_unguarded(StepType step) const {
        return this->state_with_timestamp_unguarded(step).state == State::Started;
    }

    bool is_done_unguarded(StepType step) const {
        return this->state_with_timestamp_unguarded(step).state == State::Done;
    }

    void enable_unguarded(StepType step, bool enable) {
        m_state[step].enabled = enable;
    }

    void enable_all_unguarded(bool enable) {
        for (size_t istep = 0; istep < COUNT; ++ istep)
            m_state[istep].enabled = enable;
    }

    bool is_enabled_unguarded(StepType step) const {
        return this->state_with_timestamp_unguarded(step).enabled;
    }

    // Set the step as started. Block on mutex while the Print / PrintObject / PrintRegion objects are being
    // modified by the UI thread.
    // This is necessary to block until the Print::apply() updates its state, which may
    // influence the processing step being entered.
    // Returns false if the step is not enabled or if the step has already been finished (it is done).
    template<typename ThrowIfCanceled>
    bool set_started(StepType step, std::mutex &mtx, ThrowIfCanceled throw_if_canceled) {
        std::scoped_lock<std::mutex> lock(mtx);
        // If canceled, throw before changing the step state.
        throw_if_canceled();

        PrintStateBase::StateWithTimeStamp &state = m_state[step];
        if (! state.enabled || state.state == State::Done)
            return false;
        state.state = State::Started;
        state.timestamp = ++ g_last_timestamp;
        return true;
    }

    // Set the step as done. Block on mutex while the Print / PrintObject / PrintRegion objects are being
    // modified by the UI thread.
    // Returns the timestamp when this step entered the Done state.
	template<typename ThrowIfCanceled>
	TimeStamp set_done(StepType step, std::mutex &mtx, ThrowIfCanceled throw_if_canceled) {
        std::scoped_lock<std::mutex> lock(mtx);
        // If canceled, throw before changing the step state.
        throw_if_canceled();
        assert(m_state[step].state == State::Started);
        PrintStateBase::StateWithTimeStamp &state = m_state[step];
        state.state = State::Done;
        state.timestamp = ++ g_last_timestamp;
        return state.timestamp;
    }

    // Make the step invalid.
    // PrintBase::m_state_mutex should be locked at this point, guarding access to m_state.
    // In case the step has already been entered or finished, cancel the background
    // processing by calling the cancel callback.
    template<typename CancelationCallback>
    bool invalidate(StepType step, CancelationCallback cancel) {
        if (PrintStateBase::StateWithTimeStamp &state = m_state[step]; state.try_invalidate()) {
            // Raise the mutex, so that the following cancel() callback could cancel
            // the background processing.
            cancel();
            return true;
        } else
            return false;
    }

    template<typename CancelationCallback, typename StepTypeIterator>
    bool invalidate_multiple(StepTypeIterator step_begin, StepTypeIterator step_end, CancelationCallback cancel) {
        bool invalidated = false;
        for (StepTypeIterator it = step_begin; it != step_end; ++ it)
            if (m_state[*it].try_invalidate())
                invalidated = true;
        if (invalidated) {
            // Raise the mutex, so that the following cancel() callback could cancel
            // the background processing.
            cancel();
        }
        return invalidated;
    }

    // Make all steps invalid.
    // PrintBase::m_state_mutex should be locked at this point, guarding access to m_state.
    // In case any step has already been entered or finished, cancel the background
    // processing by calling the cancel callback.
    template<typename CancelationCallback>
    bool invalidate_all(CancelationCallback cancel) {
        bool invalidated = false;
        for (size_t i = 0; i < COUNT; ++ i)
            if (m_state[i].try_invalidate())
                invalidated = true;
        if (invalidated) {
            cancel();
        }
        return invalidated;
    }

    // If the milestone is Canceled or Invalidated, return true and turn the state of the milestone to Fresh.
    // The caller is responsible for releasing the data of the milestone that is no more valid.
    bool query_reset_dirty_unguarded(StepType step) {
        if (PrintStateBase::StateWithTimeStamp &state = m_state[step]; state.is_dirty()) {
            state.state = State::Fresh;
            return true;
        } else
            return false;
    }

    // To be called after the background thread was stopped by the user pressing the Cancel button,
    // which in turn stops the background thread without adjusting state of the milestone being executed.
    // This method fixes the state of the canceled milestone by setting it to a Canceled state.
    void mark_canceled_unguarded() {
        for (size_t i = 0; i < COUNT; ++ i) {
            if (State &state = m_state[i].state; state == State::Started)
                state = State::Canceled;
        }
    }

public:
    StateWithTimeStamp m_state[COUNT];
};

class PrintBase;

class PrintObjectBase : public Domain::ObjectBase
{
public:
    const Domain::ModelObject* model_object() const    { return m_model_object; }
    Domain::ModelObject*       model_object()          { return m_model_object; }

protected:
    PrintObjectBase(Domain::ModelObject *model_object) : m_model_object(model_object) {}
    virtual ~PrintObjectBase() {}
    // Declared here to allow access from PrintBase through friendship.
	static std::mutex&                  state_mutex(PrintBase *print);
	static std::function<void()>        cancel_callback(PrintBase *print);

    Domain::ModelObject                *m_model_object;
};

// Wrapper around the private PrintBase.throw_if_canceled(), so that a cancellation object could be passed
// to a non-friend of PrintBase by a PrintBase derived object.
class PrintTryCancel
{
public:
    // calls print.throw_if_canceled().
    void operator()() const;
private:
    friend PrintBase;
    PrintTryCancel() = delete;
    PrintTryCancel(const PrintBase *print) : m_print(print) {}
    const PrintBase *m_print;
};

/**
 * @brief Printing involves slicing and export of device dependent instructions.
 *
 * Every technology has a potentially different set of requirements for
 * slicing, support structures and output print instructions. The pipeline
 * however remains roughly the same:
 *      slice -> convert to instructions -> send to printer
 *
 * The PrintBase class will abstract this flow for different technologies.
 *
 */
class PrintBase : public Domain::ObjectBase, public Biz::Slicing::IPrint
{
public:
	PrintBase() { this->restart(); }
    inline virtual ~PrintBase() {}

    virtual Domain::PrinterTechnology technology() const noexcept = 0;

    // Reset the print status including the copy of the Model / ModelObject hierarchy.
    virtual void            clear() = 0;
    // List of existing PrintObject IDs, to remove notifications for non-existent IDs.
    virtual std::vector<Domain::ObjectID> print_object_ids() const = 0;

    const Domain::Model&                  model() const { return m_model; }
    std::optional<Domain::ModelWipeTower> wipe_tower() const {
        return m_wipe_tower;
    }

    std::optional<std::reference_wrapper<const Domain::CustomGCode::Info>> custom_gcode() const {
        return m_custom_gcode;
    }

    struct TaskParams {
		TaskParams() : single_model_object(0), single_model_instance_only(false), to_object_step(-1), to_print_step(-1) {}
        // If non-empty, limit the processing to this ModelObject.
        Domain::ObjectID        single_model_object;
		// If set, only process single_model_object. Otherwise process everything, but single_model_object first.
		bool					single_model_instance_only;
        // If non-negative, stop processing at the successive object step.
        int                     to_object_step;
        // If non-negative, stop processing at the successive print step.
        int                     to_print_step;
    };
    // After calling the apply() function, call set_task() to limit the task to be processed by process().
    virtual void            set_task(const TaskParams &params) = 0;
    // Perform the calculation. This is the only method that is to be called at a worker thread.
    virtual void process() = 0;
    // Clean up after process() finished, either with success, error or if canceled.
    // The adjustments on the Print / PrintObject data due to set_task() are to be reverted here.
    virtual void            finalize() = 0;
    // Clean up print step / print object step data after
    // 1) some print step / print object step was invalidated inside PrintBase::apply() while holding the milestone mutex locked.
    // 2) background thread finished being canceled.
    virtual void            cleanup() = 0;

    // Calls a registered callback to update the status.
    void                    set_status(Domain::Percentage percent, Biz::Slicing::ProgressInfo message) {
        progress_callback(Biz::Slicing::Progress{percent, message});
    }

    typedef std::function<void()>  cancel_callback_type;
    // Various methods will call this callback to stop the background processing (the Print::process() call)
    // in case a successive change of the Print / PrintObject / PrintRegion instances changed
    // the state of the finished or running calculations.
    void                       set_cancel_callback(cancel_callback_type cancel_callback) { m_cancel_callback = cancel_callback; }
    // Has the calculation been canceled?
	enum CancelStatus {
		// No cancelation, background processing should run.
		NOT_CANCELED = 0,
		// Canceled by user from the user interface (user pressed the "Cancel" button or user closed the application).
		CANCELED_BY_USER = 1,
		// Canceled internally from Print::apply() through the Print/PrintObject::invalidate_step() or ::invalidate_all_steps().
		CANCELED_INTERNAL = 2
	};
    CancelStatus               cancel_status() const { return m_cancel_status.load(std::memory_order_acquire); }
    // Has the calculation been canceled?
	bool                       canceled() const { return m_cancel_status.load(std::memory_order_acquire) != NOT_CANCELED; }
    // Cancel the running computation. Stop execution of all the background threads.
	void                       cancel() { m_cancel_status = CANCELED_BY_USER; }
	void                       cancel_internal() { m_cancel_status = CANCELED_INTERNAL; }
    // Cancel the running computation. Stop execution of all the background threads.
	void                       restart() { m_cancel_status = NOT_CANCELED; }
    // Returns true if the last step was finished with success.
    virtual bool               finished() const = 0;

    const Biz::Parser::PlaceholderParser& placeholder_parser() const {
        ASSERT(m_placeholder_parser, "Placeholder parser must be initialized before usage!");
        return *m_placeholder_parser;
    }

protected:
	friend class PrintObjectBase;

    std::mutex&            state_mutex() const { return m_state_mutex; }
    std::function<void()>  cancel_callback() { return m_cancel_callback; }
	void				   call_cancel_callback() { m_cancel_callback(); }

    // If the background processing stop was requested, throw CanceledException.
    // To be called by the worker thread and its sub-threads (mostly launched on the TBB thread pool) regularly.
    void                   throw_if_canceled() const {
        if (stop_token.stop_requested()) {
            throw Biz::Slicing::CanceledException();
        }
        if (m_cancel_status.load(std::memory_order_acquire)) throw Biz::Slicing::CanceledException();
    }
    // Wrapper around this->throw_if_canceled(), so that throw_if_canceled() may be passed to a function without making throw_if_canceled() public.
    PrintTryCancel         make_try_cancel() const { return PrintTryCancel(this); }

    // To be called by this->output_filename() with the format string pulled from the configuration layer.
    std::string            output_filename(const std::string &format, const std::string &default_ext, const std::string &filename_base, const Biz::Parser::IO::Config *config_override = nullptr) const;
    // Update "scale", "input_filename", "input_filename_base" placeholders from the current printable ModelObjects.
    Biz::Parser::IO::Config get_object_placeholders() const;

    Domain::Model                           m_model;
    std::optional<Domain::ModelWipeTower>   m_wipe_tower;
    std::optional<Domain::CustomGCode::Info>m_custom_gcode;

    std::optional<Biz::Parser::PlaceholderParser>        m_placeholder_parser;

private:
    std::atomic<CancelStatus>               m_cancel_status;

    // Callback to be evoked to stop the background processing before a state is updated.
    cancel_callback_type                    m_cancel_callback = [](){};

    // Mutex used for synchronization of the worker thread with the UI thread:
    // The mutex will be used to guard the worker thread against entering a stage
    // while the data influencing the stage is modified.
    mutable std::mutex                      m_state_mutex;

    friend PrintTryCancel;
};

template<typename PrintStepEnumType, const size_t COUNT>
class PrintBaseWithState : public PrintBase
{
public:
    using                           PrintStepEnum       = PrintStepEnumType;
    static constexpr const size_t   PrintStepEnumSize   = COUNT;

    PrintBaseWithState() = default;

    bool            is_step_done(PrintStepEnum step) const { return m_state.is_done(step, this->state_mutex()); }
	PrintStateBase::StateWithTimeStamp step_state_with_timestamp(PrintStepEnum step) const { return m_state.state_with_timestamp(step, this->state_mutex()); }

protected:
    bool            set_started(PrintStepEnum step) { return m_state.set_started(step, this->state_mutex(), [this](){ this->throw_if_canceled(); }); }
	PrintStateBase::TimeStamp set_done(PrintStepEnum step) {
		return m_state.set_done(step, this->state_mutex(), [this](){ this->throw_if_canceled(); });
	}
    bool            invalidate_step(PrintStepEnum step)
		{ return m_state.invalidate(step, this->cancel_callback()); }
    template<typename StepTypeIterator>
    bool            invalidate_steps(StepTypeIterator step_begin, StepTypeIterator step_end) 
        { return m_state.invalidate_multiple(step_begin, step_end, this->cancel_callback()); }
    bool            invalidate_steps(std::initializer_list<PrintStepEnum> il) 
        { return m_state.invalidate_multiple(il.begin(), il.end(), this->cancel_callback()); }
    bool            invalidate_all_steps() 
        { return m_state.invalidate_all(this->cancel_callback()); }

	bool            is_step_started_unguarded(PrintStepEnum step) const { return m_state.is_started_unguarded(step); }
	bool            is_step_done_unguarded(PrintStepEnum step) const { return m_state.is_done_unguarded(step); }

    // After calling the apply() function, set_task() may be called to limit the task to be processed by process().
    template<typename PrintObject>
    void set_task_impl(const TaskParams &params, std::vector<PrintObject*> &print_objects)
    {
        static constexpr const auto PrintObjectStepEnumSize = int(PrintObject::PrintObjectStepEnumSize);
        using                       PrintObjectStepEnum     = typename PrintObject::PrintObjectStepEnum;
        // Grab the lock for the Print / PrintObject milestones.
        std::scoped_lock<std::mutex> lock(this->state_mutex());

        int n_object_steps = int(params.to_object_step) + 1;
        if (n_object_steps == 0)
            n_object_steps = PrintObjectStepEnumSize;

        if (params.single_model_object.valid()) {
            // Find the print object to be processed with priority.
            PrintObject *print_object = nullptr;
            size_t       idx_print_object = 0;
            for (; idx_print_object < print_objects.size(); ++ idx_print_object)
                if (print_objects[idx_print_object]->model_object()->id() == params.single_model_object) {
                    print_object = print_objects[idx_print_object];
                    break;
                }
            assert(print_object != nullptr);
            // Find out whether the priority print object is being currently processed.
            bool running = false;
            for (int istep = 0; istep < n_object_steps; ++ istep) {
                if (! print_object->is_step_enabled_unguarded(PrintObjectStepEnum(istep)))
                    // Step was skipped, cancel.
                    break;
                if (print_object->is_step_started_unguarded(PrintObjectStepEnum(istep))) {
                    // No step was skipped, and a wanted step is being processed. Don't cancel.
                    running = true;
                    break;
                }
            }
            if (! running)
                this->call_cancel_callback();

            // Now the background process is either stopped, or it is inside one of the print object steps to be calculated anyway.
            if (params.single_model_instance_only) {
                // Suppress all the steps of other instances.
                for (PrintObject *po : print_objects)
                    for (size_t istep = 0; istep < PrintObjectStepEnumSize; ++ istep)
                        po->enable_step_unguarded(PrintObjectStepEnum(istep), false);
            } else if (! running) {
                // Swap the print objects, so that the selected print_object is first in the row.
                // At this point the background processing must be stopped, so it is safe to shuffle print objects.
                if (idx_print_object != 0)
                    std::swap(print_objects.front(), print_objects[idx_print_object]);
            }
            // and set the steps for the current object.
            for (int istep = 0; istep < n_object_steps; ++ istep)
                print_object->enable_step_unguarded(PrintObjectStepEnum(istep), true);
            for (int istep = n_object_steps; istep < PrintObjectStepEnumSize; ++ istep)
                print_object->enable_step_unguarded(PrintObjectStepEnum(istep), false);
        } else {
            // Slicing all objects.
            bool running = false;
            for (PrintObject *print_object : print_objects)
                for (int istep = 0; istep < n_object_steps; ++ istep) {
                    if (! print_object->is_step_enabled_unguarded(PrintObjectStepEnum(istep))) {
                        // Step may have been skipped. Restart.
                        goto loop_end;
                    }
                    if (print_object->is_step_started_unguarded(PrintObjectStepEnum(istep))) {
                        // This step is running, and the state cannot be changed due to the this->state_mutex() being locked.
                        // It is safe to manipulate m_stepmask of other PrintObjects and Print now.
                        running = true;
                        goto loop_end;
                    }
                }
        loop_end:
            if (! running)
                this->call_cancel_callback();
            for (PrintObject *po : print_objects) {
                for (int istep = 0; istep < n_object_steps; ++ istep)
                    po->enable_step_unguarded(PrintObjectStepEnum(istep), true);
                for (int istep = n_object_steps; istep < PrintObjectStepEnumSize; ++ istep)
                    po->enable_step_unguarded(PrintObjectStepEnum(istep), false);
            }
        }

        if (params.to_object_step != -1 || params.to_print_step != -1) {
            // Limit the print steps.
            size_t istep = (params.to_object_step != -1) ? 0 : size_t(params.to_print_step) + 1;
            for (; istep < PrintStepEnumSize; ++ istep)
                m_state.enable_unguarded(PrintStepEnum(istep), false);
        }
    }

    /**
     * @brief Limits the processing to the given model object and stops after the given print object step.
     *
     * @return False when the model object has no print object.
     */
    template <typename PrintObject>
    bool set_task_until_object_step_impl(
        const int to_object_step,
        const Domain::ObjectID model_object_id,
        std::vector<PrintObject*>& print_objects
    )
    {
        const bool model_object_found = std::any_of(
            print_objects.begin(),
            print_objects.end(),
            [&model_object_id](const PrintObject* print_object)
            { return print_object->model_object()->id() == model_object_id; }
        );
        if (!model_object_found) {
            return false;
        }

        TaskParams task;
        task.single_model_object        = model_object_id;
        task.single_model_instance_only = true;
        task.to_object_step             = to_object_step;
        this->set_task_impl(task, print_objects);

        return true;
    }

    // Clean up after process() finished, either with success, error or if canceled.
    // The adjustments on the Print / PrintObject m_stepmask data due to set_task() are to be reverted here:
    // Execution of all milestones is enabled in case some of them were suppressed for the last background execution.
    // Also if the background processing was canceled, the current milestone that was just abandoned 
    // in Started state is to be reset to Canceled state.
    template<typename PrintObject>
    void finalize_impl(std::vector<PrintObject*> &print_objects)
    {
        // Grab the lock for the Print / PrintObject milestones.
        std::scoped_lock<std::mutex> lock(this->state_mutex());
        for (auto *po : print_objects)
            po->finalize_impl();
        m_state.enable_all_unguarded(true);
        m_state.mark_canceled_unguarded();
    }

public:
    PrintState<PrintStepEnum, COUNT>    m_state;
};

template<typename PrintType, typename PrintObjectStepEnumType, const size_t COUNT>
class PrintObjectBaseWithState : public PrintObjectBase
{
public:
    using                           PrintObjectStepEnum       = PrintObjectStepEnumType;
    static constexpr const size_t   PrintObjectStepEnumSize   = COUNT;

    PrintType*       print()         { return m_print; }
    const PrintType* print() const   { return m_print; }

    typedef PrintState<PrintObjectStepEnum, COUNT> PrintObjectState;
    bool            is_step_done(PrintObjectStepEnum step) const { return m_state.is_done(step, PrintObjectBase::state_mutex(m_print)); }
    PrintStateBase::StateWithTimeStamp step_state_with_timestamp(PrintObjectStepEnum step) const { return m_state.state_with_timestamp(step, PrintObjectBase::state_mutex(m_print)); }

    bool all_steps_done() const {
        return is_step_done(PrintObjectStepEnum(int(COUNT) - 1));
    }

    auto last_completed_step() const
    {
        static_assert(COUNT > 0, "Step count should be > 0");
        auto s = int(COUNT) - 1;

        std::lock_guard lk(state_mutex(m_print));
        while (s >= 0 && ! is_step_done_unguarded(PrintObjectStepEnum(s)))
            --s;

        if (s < 0)
            s = COUNT;

        return PrintObjectStepEnum(s);
    }

protected:
	PrintObjectBaseWithState(PrintType *print, Domain::ModelObject *model_object) : PrintObjectBase(model_object), m_print(print) {}

    bool            set_started(PrintObjectStepEnum step)
        { return m_state.set_started(step, PrintObjectBase::state_mutex(m_print), [this](){ this->throw_if_canceled(); }); }
	PrintStateBase::TimeStamp set_done(PrintObjectStepEnum step) {
		return m_state.set_done(step, PrintObjectBase::state_mutex(m_print), [this](){ this->throw_if_canceled(); });
	}

    bool            invalidate_step(PrintObjectStepEnum step)
        { return m_state.invalidate(step, PrintObjectBase::cancel_callback(m_print)); }
    template<typename StepTypeIterator>
    bool            invalidate_steps(StepTypeIterator step_begin, StepTypeIterator step_end) 
        { return m_state.invalidate_multiple(step_begin, step_end, PrintObjectBase::cancel_callback(m_print)); }
    bool            invalidate_steps(std::initializer_list<PrintObjectStepEnum> il) 
        { return m_state.invalidate_multiple(il.begin(), il.end(), PrintObjectBase::cancel_callback(m_print)); }
    bool            invalidate_all_steps() 
        { return m_state.invalidate_all(PrintObjectBase::cancel_callback(m_print)); }

    bool            is_step_started_unguarded(PrintObjectStepEnum step) const { return m_state.is_started_unguarded(step); }
    bool            is_step_done_unguarded(PrintObjectStepEnum step) const { return m_state.is_done_unguarded(step); }

    bool            is_step_enabled_unguarded(PrintObjectStepEnum step) const { return m_state.is_enabled_unguarded(step); }
    void            enable_step_unguarded(PrintObjectStepEnum step, bool enable) { m_state.enable_unguarded(step, enable); }
    void            enable_all_steps_unguarded(bool enable) { m_state.enable_all_unguarded(enable); }
    // See the comment at PrintBaseWithState::finalize_impl()
    void            finalize_impl() { m_state.enable_all_unguarded(true); m_state.mark_canceled_unguarded(); }
    // If the milestone is Canceled or Invalidated, return true and turn the state of the milestone to Fresh.
    // The caller is responsible for releasing the data of the milestone that is no more valid.
    bool            query_reset_dirty_step_unguarded(PrintObjectStepEnum step) { return m_state.query_reset_dirty_unguarded(step); }

protected:
    // If the background processing stop was requested, throw CanceledException.
    // To be called by the worker thread and its sub-threads (mostly launched on the TBB thread pool) regularly.
    void            throw_if_canceled() { if (m_print->canceled()) throw Biz::Slicing::CanceledException(); }

    friend PrintType;
    PrintType                                *m_print;

public:
    PrintState<PrintObjectStepEnum, COUNT>    m_state;
};

} // namespace Slic3r

#endif /* slic3r_PrintBase_hpp_ */

module storage.state;
import std;

namespace
{
    thread_local bool gui_thread{};
    class executor_t
    {
        std::mutex mutex;
        std::condition_variable_any changed;
        std::deque<std::function<void()>> work;
        // Join after the queue and locks: durable writes drain on normal exit.
        std::jthread thread{[this](std::stop_token stop)
        {
            for (;;)
            {
                std::unique_lock lock{mutex};
                changed.wait(lock,stop,[this]{return !work.empty();});
                if (work.empty()) return;
                auto next=std::move(work.front());work.pop_front();
                lock.unlock();
                next();
            }
        }};
      public:
        auto add(std::function<void()> value) -> void
        {
            {std::lock_guard lock{mutex};work.push_back(std::move(value));}
            changed.notify_one();
        }
    };
    auto executor() -> executor_t& {static executor_t value;return value;}
    auto journal_executor() -> executor_t& {static executor_t value;return value;}
}
auto storage_gui_thread_set() -> void {gui_thread=true;}
auto storage_is_gui_thread() -> bool {return gui_thread;}
auto storage_background(std::function<void()> work) -> std::shared_future<void>
{
    auto task=std::make_shared<std::packaged_task<void()>>(std::move(work));
    auto result=task->get_future().share();
    executor().add([task]{(*task)();});
    return result;
}

struct storage_write_queue_t::implementation_t : std::enable_shared_from_this<implementation_t>
{
    mutable std::mutex mutex;
    mutable std::condition_variable changed;
    std::deque<std::function<void()>> writes;
    std::exception_ptr error;
    bool running{};
    auto start_locked() -> void
    {
        if(running || error || writes.empty())return;
        running=true;
        journal_executor().add([self=shared_from_this()]
        {
            for(;;)
            {
                std::function<void()> work;
                {
                    std::lock_guard lock{self->mutex};
                    if(self->writes.empty()){self->running=false;self->changed.notify_all();return;}
                    work=self->writes.front();
                }
                try{work();}
                catch(...)
                {
                    std::lock_guard lock{self->mutex};
                    // Keep the failed write and everything after it. Retry must
                    // never skip a failed ID reservation/checkpoint/index edit.
                    self->error=std::current_exception();self->running=false;
                    self->changed.notify_all();return;
                }
                {std::lock_guard lock{self->mutex};self->writes.pop_front();}
            }
        });
    }
};
storage_write_queue_t::storage_write_queue_t():m_impl{std::make_shared<implementation_t>()}{}
auto storage_write_queue_t::push(std::function<void()> work) -> void
{
    std::lock_guard lock{m_impl->mutex};m_impl->writes.push_back(std::move(work));m_impl->start_locked();
}
auto storage_write_queue_t::ready() const -> bool
{
    std::lock_guard lock{m_impl->mutex};
    if(m_impl->error)std::rethrow_exception(m_impl->error);
    return m_impl->writes.empty() && !m_impl->running;
}
auto storage_write_queue_t::wait() const -> void
{
    std::unique_lock lock{m_impl->mutex};
    m_impl->changed.wait(lock,[&]{return m_impl->error || (m_impl->writes.empty() && !m_impl->running);});
    if(m_impl->error)std::rethrow_exception(m_impl->error);
}
auto storage_write_queue_t::retry() -> void
{
    std::lock_guard lock{m_impl->mutex};m_impl->error=nullptr;m_impl->start_locked();
}

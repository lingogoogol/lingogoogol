export module storage.http.curl.worker;

import std;

import external.curl;
import storage.http.curl.easy;

export
{
    // The creating (GUI) thread owns the facade. Only its worker may access the
    // multi handle or an in-flight easy handle; queues transfer sole ownership.
    class curl_multi_worker_t
    {
        struct implementation_t;
        std::unique_ptr<implementation_t> m_impl;

        auto submit(std::unique_ptr<curl_easy_t> easy) -> void;
    public:
        // Resume a parked request without slicing its dynamic batch type.
        auto add(std::unique_ptr<curl_easy_t> easy) -> void { submit(std::move(easy)); }
        curl_multi_worker_t();
        curl_multi_worker_t(const curl_multi_worker_t&) = delete;
        auto operator=(const curl_multi_worker_t&) -> curl_multi_worker_t& = delete;
        ~curl_multi_worker_t();

        template<typename t_easy>
        auto add(t_easy&& easy) -> void
        {
            submit(std::make_unique<std::remove_cvref_t<t_easy>>(std::forward<t_easy>(easy)));
        }

        // Check worker errors and collect its retry count without running curl.
        auto run() -> std::uint64_t;
        auto read_progress(CURL* key) const -> std::uint64_t;
        auto ready_nonempty() const -> bool;
        auto ready_get_unique() -> std::unique_ptr<curl_easy_t>;

        // Interrupt socket waits, join the worker and discard pending results.
        // Idempotent; call before destroying transfer tasks or GUI resources.
        auto stop() -> void;
        auto interrupt() -> void;
        auto pending(const std::shared_ptr<request_control_t>& control) const -> bool;
    };
}

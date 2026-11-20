module;

#include "android/native/activity.hpp"

#include <cerrno>
#include <unistd.h>

module storage.local;

import std;

// Storage Access Framework documents through MainActivity.kt (LocalFiles):
// the activity resolves URIs and "<directory URI>|<relative/path>" targets and
// hands back file descriptors; reading and writing stay in C++.
namespace
{
    // A seekable stream over a descriptor from ParcelFileDescriptor.detachFd().
    class descriptor_buffer_t : public std::streambuf
    {
        int m_descriptor;
        std::array<char, 1 << 16> m_buffer{};
        std::uint64_t m_position{}; // File offset of the start of the buffer.
        bool m_writing{};

        auto flush_output() -> bool
        {
            for (auto* data{pbase()}; data < pptr();)
            {
                const auto written{::pwrite(m_descriptor, data, static_cast<std::size_t>(pptr() - data),
                                            static_cast<off_t>(m_position))};
                if (written < 0 && errno == EINTR) continue;
                if (written <= 0) return false;
                data += written;
                m_position += static_cast<std::uint64_t>(written);
            }
            setp(m_buffer.data(), m_buffer.data() + m_buffer.size());
            return true;
        }

      protected:
        auto underflow() -> int_type override
        {
            if (m_writing && !flush_output()) return traits_type::eof();
            m_writing = false;
            m_position += static_cast<std::uint64_t>(egptr() - eback());
            for (;;)
            {
                const auto read{::pread(m_descriptor, m_buffer.data(), m_buffer.size(), static_cast<off_t>(m_position))};
                if (read < 0 && errno == EINTR) continue;
                if (read <= 0)
                {
                    setg(m_buffer.data(), m_buffer.data(), m_buffer.data());
                    return traits_type::eof();
                }
                setg(m_buffer.data(), m_buffer.data(), m_buffer.data() + read);
                return traits_type::to_int_type(m_buffer[0]);
            }
        }
        auto overflow(int_type character) -> int_type override
        {
            if (!m_writing)
            {
                m_position += static_cast<std::uint64_t>(gptr() - eback());
                setg(nullptr, nullptr, nullptr);
                setp(m_buffer.data(), m_buffer.data() + m_buffer.size());
                m_writing = true;
            }
            else if (!flush_output())
                return traits_type::eof();
            if (!traits_type::eq_int_type(character, traits_type::eof()))
            {
                *pptr() = traits_type::to_char_type(character);
                pbump(1);
            }
            return traits_type::not_eof(character);
        }
        auto sync() -> int override { return m_writing && !flush_output() ? -1 : 0; }
        auto seekoff(off_type offset, std::ios_base::seekdir direction, std::ios_base::openmode) -> pos_type override
        {
            if (m_writing && !flush_output()) return pos_type(off_type(-1));
            const auto current{m_position + static_cast<std::uint64_t>(m_writing ? 0 : gptr() - eback())};
            std::int64_t target{offset};
            if (direction == std::ios_base::cur) target += static_cast<std::int64_t>(current);
            else if (direction == std::ios_base::end) target += ::lseek(m_descriptor, 0, SEEK_END);
            if (target < 0) return pos_type(off_type(-1));
            m_position = static_cast<std::uint64_t>(target);
            m_writing = false;
            setg(nullptr, nullptr, nullptr);
            setp(nullptr, nullptr);
            return pos_type(target);
        }
        auto seekpos(pos_type position, std::ios_base::openmode mode) -> pos_type override
        {
            return seekoff(off_type(position), std::ios_base::beg, mode);
        }

      public:
        explicit descriptor_buffer_t(int descriptor) : m_descriptor{descriptor} {}
        ~descriptor_buffer_t() override
        {
            sync();
            ::close(m_descriptor);
        }
    };

    template <typename t_stream> class descriptor_stream_t : public t_stream
    {
        descriptor_buffer_t m_buffer;

      public:
        explicit descriptor_stream_t(int descriptor) : t_stream{nullptr}, m_buffer{descriptor}
        {
            this->rdbuf(&m_buffer);
            if (descriptor < 0) this->setstate(std::ios_base::failbit);
        }
        ~descriptor_stream_t() override { this->rdbuf(nullptr); }
    };

    auto call_string(const char* method, const std::string& path) -> std::string
    {
        const auto env{android_jni()};
        const auto text{android_string(env, path)};
        auto result{android_call_string(method, "(Ljava/lang/String;)Ljava/lang/String;", text)};
        env->DeleteLocalRef(text);
        return result;
    }

    auto call_boolean(const char* method, const std::string& path) -> bool
    {
        const auto env{android_jni()};
        const auto text{android_string(env, path)};
        const auto result{android_call_boolean(method, "(Ljava/lang/String;)Z", text)};
        env->DeleteLocalRef(text);
        return result;
    }

    // "<kind> <size> <modified>"; kind 0 = missing, 1 = file, 2 = directory, 3 = other.
    struct stat_t
    {
        int kind{};
        std::uint64_t size{};
        std::int64_t modified{};
    };
    auto stat(const std::string& path) -> stat_t
    {
        stat_t result;
        std::istringstream{call_string("localStat", path)} >> result.kind >> result.size >> result.modified;
        return result;
    }

    auto open(const std::string& path, const char* mode) -> int
    {
        const auto env{android_jni()};
        const auto text{android_string(env, path)};
        const auto mode_text{env->NewStringUTF(mode)};
        const auto descriptor{android_call_string("localOpen", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;",
                                                  text, mode_text)};
        env->DeleteLocalRef(mode_text);
        env->DeleteLocalRef(text);
        return descriptor.empty() ? -1 : std::stoi(descriptor);
    }
}

namespace storage_local
{
    auto children(const std::string& directory) -> std::vector<child_t>
    {
        // One entry per line: "<kind> <uri>", sorted by display name.
        std::vector<child_t> result;
        std::istringstream lines{call_string("localChildren", directory)};
        for (std::string line; std::getline(lines, line);)
        {
            if (line.size() < 3) continue;
            result.push_back({line.substr(2), line[0] == '1' ? kind_t::file : line[0] == '2' ? kind_t::directory
                                                                                                : kind_t::other});
        }
        return result;
    }

    auto exists(const std::string& path) -> bool { return stat(path).kind != 0; }

    auto size(const std::string& path) -> std::uint64_t
    {
        const auto found{stat(path)};
        if (!found.kind) throw std::runtime_error{"Local file not found: " + path};
        return found.size;
    }

    auto modified(const std::string& path) -> std::int64_t { return stat(path).modified; }

    auto name(const std::string& path) -> std::string { return call_string("localName", path); }

    auto display(const std::string& path) -> std::string { return call_string("localDisplay", path); }

    auto child(const std::string& directory, const std::string& name) -> std::string
    {
        return directory + (directory.contains('|') ? "/" : "|") + name;
    }

    auto open_read(const std::string& path) -> std::unique_ptr<std::istream>
    {
        return std::make_unique<descriptor_stream_t<std::istream>>(open(path, "r"));
    }

    auto open_write(const std::string& path) -> std::unique_ptr<std::ostream>
    {
        return std::make_unique<descriptor_stream_t<std::ostream>>(open(path, "rw"));
    }

    auto create(const std::string& path) -> bool
    {
        const auto descriptor{open(path, "rwt")};
        if (descriptor < 0) return false;
        ::close(descriptor);
        return true;
    }

    auto resize(const std::string& path, std::uint64_t size) -> void
    {
        const auto descriptor{open(path, "rw")};
        const bool done{descriptor >= 0 && ::ftruncate(descriptor, static_cast<off_t>(size)) == 0};
        if (descriptor >= 0) ::close(descriptor);
        if (!done) throw std::runtime_error{"Cannot resize download file: " + path};
    }

    auto flush(const std::string& path) -> void
    {
        const auto descriptor{open(path, "rw")};
        const bool done{descriptor >= 0 && ::fsync(descriptor) == 0};
        if (descriptor >= 0) ::close(descriptor);
        if (!done) throw std::runtime_error{"Cannot flush download: " + path};
    }

    auto create_directories(const std::string& path) -> void
    {
        if (!call_boolean("localMakeDirectories", path))
            throw std::runtime_error{"Cannot create download folder: " + path};
    }

    auto remove_all(const std::string& path) -> void
    {
        if (!call_boolean("localRemove", path))
            throw std::runtime_error{"Cannot remove: " + path};
    }
}

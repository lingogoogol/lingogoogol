#pragma once
#include "blob_format.hpp"
#include <functional>
#include <limits>
#include <set>
#include <string>
#include <vector>

namespace storage_format
{
    struct summary_t { std::uint64_t size{}, minimum{}; nlohmann::json name; };
    using read_blob_t = std::function<nlohmann::json(const std::string&)>;
    using write_blob_t = std::function<void(const std::string&,const nlohmann::json&,const nlohmann::json&)>;

    inline auto checked_add(std::uint64_t& total, std::uint64_t size) -> void
    {
        if (size > std::numeric_limits<std::uint64_t>::max()-total)
            throw std::runtime_error{"Folder size overflow."};
        total += size;
    }

    // Reading old metadata never fabricates timestamps or mutates cloud data.
    // Missing folder sizes are computed recursively, with cycle detection.
    inline auto summarize(const std::string& id, const std::string& type, const read_blob_t& read,
                          std::set<std::string>& active) -> summary_t
    {
        if (!active.insert(id).second) throw std::runtime_error{"Cyclic folder metadata."};
        struct erase_t { std::set<std::string>& set; std::string id; ~erase_t(){set.erase(id);} } erase{active,id};
        auto blob = read(id);
        auto name = nlohmann::json::parse(blob.at("name").get<std::string>());
        auto min = minimum(name);
        if (type == "file" || (version(name) >= 2 && name.contains("size") && name.contains("version_min")))
            return {name.at("size").get<std::uint64_t>(),min,std::move(name)};
        std::uint64_t size{};
        const auto main_name = name;
        std::set<std::string> pages;
        auto page = id;
        for (;;)
        {
            if (!pages.insert(page).second) throw std::runtime_error{"Cyclic folder pages."};
            min = std::min(min,version(name));
            const auto entries = nlohmann::json::parse(blob.at("description").get<std::string>());
            if (!entries.is_array()) throw std::runtime_error{"Invalid folder index."};
            for (const auto& child : entries)
            {
                const auto summary = summarize(child.at("id"),child.at("type"),read,active);
                checked_add(size,summary.size);
                min = std::min(min,summary.minimum);
            }
            page = name.at("next").get<std::string>();
            if (page == "null") break;
            blob = read(page);
            name = nlohmann::json::parse(blob.at("name").get<std::string>());
        }
        return {size,min,main_name};
    }
    inline auto summarize(const std::string& id, const std::string& type, const read_blob_t& read) -> summary_t
    { std::set<std::string> active; return summarize(id,type,read,active); }

    // Recompute absolute totals instead of adding deltas: retries after a lost
    // PATCH response must not count a pasted/uploaded entity twice. Mutations
    // run under the existing publication chain, so concurrent jobs cannot lose
    // each other's entries. Complete all pages before walking to the parent.
    inline auto refresh_ancestors(std::string id, const read_blob_t& read, const write_blob_t& write) -> void
    {
        std::set<std::string> ancestors;
        while (id != "null")
        {
            if (!ancestors.insert(id).second) throw std::runtime_error{"Cyclic folder parents."};
            struct page_t { std::string id; nlohmann::json name, entries; };
            std::vector<page_t> pages;
            std::set<std::string> visited;
            auto page = id;
            std::uint64_t size{}, min = current_version;
            std::string parent;
            while (page != "null")
            {
                if (!visited.insert(page).second) throw std::runtime_error{"Cyclic folder pages."};
                auto blob = read(page);
                auto name = nlohmann::json::parse(blob.at("name").get<std::string>());
                version(name);
                if (pages.empty()) parent = name.at("parent").get<std::string>();
                auto entries = nlohmann::json::parse(blob.at("description").get<std::string>());
                if (!entries.is_array()) throw std::runtime_error{"Invalid folder index."};
                for (auto& child : entries)
                {
                    std::set<std::string> active{id};
                    const auto summary = summarize(child.at("id"),child.at("type"),read,active);
                    checked_add(size,summary.size);
                    min = std::min(min,summary.minimum);
                }
                pages.push_back({page,std::move(name),std::move(entries)});
                page = pages.back().name.at("next").get<std::string>();
            }
            // Write the main page last; retries always re-read child metadata.
            for (auto i = pages.rbegin(); i != pages.rend(); ++i)
            {
                if (!i->name.contains("time")) i->name["time"] = now();
                i->name["version"] = current_version;
                i->name["type"] = "folder";
                i->name["size"] = size;
                i->name["version_min"] = min;
                write(i->id,i->name,i->entries);
            }
            id = parent;
        }
    }
}

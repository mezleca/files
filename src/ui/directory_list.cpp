#include "directory_list.hpp"

#include "directory_entry.hpp"

#include <ui/tree/node.hpp>
#include <ui/widgets/text.hpp>
#include <utility>

using namespace ui;

DirectoryList::DirectoryList() : VirtualLayout("Directory List", 32.0F) {
    set_spacing(10.0F);
    set_size({grow(), grow()});
}

void DirectoryList::set_entries(std::vector<fs::path> entries, bool has_directory) {
    clear_built_entries();
    m_entry_widgets.clear();
    m_default_entry = nullptr;
    m_pending_entry.reset();
    m_entries = std::move(entries);

    if (m_entries.empty()) {
        const char* message = has_directory ? "folder has no content" : "Hello :D";
        set_items(1, [this, message](std::size_t) -> Node& {
            if (m_default_entry == nullptr) {
                m_default_entry = &add<TextWidget>(message);
            }

            return *m_default_entry;
        });
        return;
    }

    set_items(m_entries.size(), [this](std::size_t index) -> Node& {
        const auto found = m_entry_widgets.find(index);
        if (found != m_entry_widgets.end()) {
            return *found->second;
        }

        auto& entry = add<DirectoryEntry>(m_entries[index]);
        entry.build();
        entry.set_on_click([this](const fs::path& path) {
            if (m_on_click) {
                m_pending_entry = path;
            }
        });
        m_entry_widgets.emplace(index, &entry);
        return entry;
    });
}

void DirectoryList::set_on_click(std::function<void(const fs::path&)> callback) {
    m_on_click = std::move(callback);
}

void DirectoryList::apply_theme_defaults(const Theme& theme) {
    Container::apply_theme_defaults(theme);
}

void DirectoryList::on_update(float) {
    if (!m_pending_entry.has_value()) {
        return;
    }

    fs::path path = std::move(*m_pending_entry);
    m_pending_entry.reset();
    if (m_on_click) {
        m_on_click(path);
    }
}

void DirectoryList::clear_built_entries() {
    while (!children().empty()) {
        remove(*children().back());
    }
}

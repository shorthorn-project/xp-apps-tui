#include "rebuildtui.hpp"
#include <fmt/core.h>
#include <utility>
#include "core/terminal.hpp"

namespace tui {

    NavigationTUI::NavigationTUI() :
        current_state_(NavigationState::MAIN_MENU), current_section_index_(0), current_selection_index_(0),
        current_page_(0), current_section_page_{0}, running_(false), needs_redraw_(true), previous_width_{0},
        previous_height_{0} {
        config_ = Config{};
        terminal_manager_ = std::make_unique<TerminalManager>();
    }

    NavigationTUI::NavigationTUI(Config config) :
        current_state_(NavigationState::MAIN_MENU), current_section_index_(0), current_selection_index_(0),
        current_page_(0), current_section_page_{0}, config_(std::move(config)), running_(false), needs_redraw_(true),
        previous_width_{0}, previous_height_{0} {
        terminal_manager_ = std::make_unique<TerminalManager>();
    }

    void NavigationTUI::add_section(const Section& section) { sections_.push_back(section); }

    void NavigationTUI::add_section(Section&& section) { sections_.push_back(std::move(section)); }

    void NavigationTUI::add_sections(const std::vector<Section>& sections) {
        sections_.insert(sections_.end(), sections.begin(), sections.end());
    }

    void NavigationTUI::add_sections(std::vector<Section>&& sections) {
        sections_.insert(sections_.end(), std::make_move_iterator(sections.begin()),
                         std::make_move_iterator(sections.end()));
    }

    Section* NavigationTUI::get_section(size_t index) {
        return (index < sections_.size()) ? &sections_[index] : nullptr;
    }

    const Section* NavigationTUI::get_section(size_t index) const {
        return (index < sections_.size()) ? &sections_[index] : nullptr;
    }

    Section* NavigationTUI::get_section_by_name(const std::string& name) {
#if __cplusplus >= 202002L
        const auto it =
            std::ranges::find_if(sections_, [&name](const Section& section) { return section.name == name; });
#else
        const auto it = std::find_if(sections_.begin(), sections_.end(),
                                     [&name](const Section& section) { return section.name == name; });
#endif
        return (it != sections_.end()) ? &(*it) : nullptr;
    }

    const Section* NavigationTUI::get_section_by_name(const std::string& name) const {
#if __cplusplus >= 202002L
        const auto it =
            std::ranges::find_if(sections_, [&name](const Section& section) { return section.name == name; });
#else
        const auto it = std::find_if(sections_.begin(), sections_.end(),
                                     [&name](const Section& section) { return section.name == name; });
#endif
        return (it != sections_.end()) ? &(*it) : nullptr;
    }

    size_t NavigationTUI::get_section_count() const { return sections_.size(); }

    bool NavigationTUI::remove_section(const size_t index) {
        if (index < sections_.size()) {
            using diff_t = typename decltype(sections_)::difference_type;
            sections_.erase(sections_.begin() + static_cast<diff_t>(index));
            validate_indices();
            return true;
        }
        return false;
    }

    bool NavigationTUI::remove_section_by_name(const std::string& name) {
#if __cplusplus >= 202002L
        const auto it =
            std::ranges::find_if(sections_, [&name](const Section& section) { return section.name == name; });
#else
        const auto it = std::find_if(sections_.begin(), sections_.end(),
                                     [&name](const Section& section) { return section.name == name; });
#endif
        if (it != sections_.end()) {
            sections_.erase(it);
            validate_indices();
            return true;
        }
        return false;
    }

    void NavigationTUI::clear_sections() {
        sections_.clear();
        current_section_index_ = 0;
        current_selection_index_ = 0;
        current_page_ = 0;
        current_state_ = NavigationState::MAIN_MENU;
    }

    void NavigationTUI::set_section_selected_callback(SectionSelectedCallback callback) {
        on_section_selected_ = std::move(callback);
    }

    void NavigationTUI::set_item_toggled_callback(ItemToggledCallback callback) {
        on_item_toggled_ = std::move(callback);
    }

    void NavigationTUI::set_page_changed_callback(PageChangedCallback callback) {
        on_page_changed_ = std::move(callback);
    }

    void NavigationTUI::set_state_changed_callback(StateChangedCallback callback) {
        on_state_changed_ = std::move(callback);
    }

    void NavigationTUI::set_exit_callback(ExitCallback callback) { on_exit_ = std::move(callback); }

    void NavigationTUI::set_custom_command_callback(CustomCommandCallback callback) {
        on_custom_command_ = std::move(callback);
    }

    void NavigationTUI::set_update_callback(UpdateCallback callback) { update_callback_ = std::move(callback); }

    void NavigationTUI::refresh_items() { needs_redraw_ = true; }

    void NavigationTUI::run() {
        if (sections_.empty()) {
            fmt::println("No sections available. Please add sections before running.");
            return;
        }

        initialize();
        running_ = true;

        while (running_) {
            if (needs_redraw_) {
                render();
                needs_redraw_ = false;
            }

            if (update_callback_) {
                update_callback_();
            }

            int timeout_ms = (update_callback_ != nullptr) ? 50 : 100;

            if (TerminalManager::wait_for_input(timeout_ms)) {
                process_events();
            }
        }

        terminal_manager_->restore_terminal();

        if (on_exit_) {
            on_exit_(sections_);
        }
    }

    void NavigationTUI::exit() { running_ = false; }

    NavigationTUI::NavigationState NavigationTUI::get_current_state() const { return current_state_; }

    size_t NavigationTUI::get_current_section_index() const { return current_section_index_; }

    int NavigationTUI::get_current_page() const { return current_page_; }

    size_t NavigationTUI::get_current_selection_index() const { return current_selection_index_; }

    void NavigationTUI::return_to_sections() {
        if (current_state_ != NavigationState::MAIN_MENU) {
            change_state(NavigationState::MAIN_MENU);
            current_selection_index_ = static_cast<int>(current_section_index_) % config_.layout.sections_per_page;
            current_section_page_ = static_cast<int>(current_section_index_) / config_.layout.sections_per_page;
            needs_redraw_ = true;
        }
    }

    void NavigationTUI::enter_section(const size_t section_index) {
        if (section_index < sections_.size()) {
            current_section_index_ = section_index;
            current_selection_index_ = 0;
            current_page_ = 0;
            change_state(NavigationState::ITEM_SELECTION);

            const auto& section = sections_[section_index];
            section.trigger_enter();

            if (on_section_selected_) {
                on_section_selected_(section_index, section);
            }

            needs_redraw_ = true;
        }
    }

    int NavigationTUI::get_sections_on_current_page() const {
        const int start = current_section_page_ * config_.layout.sections_per_page;
        const int end = std::min(start + config_.layout.sections_per_page, static_cast<int>(sections_.size()));
        return end - start;
    }

    void NavigationTUI::go_to_section_page(const int page) {
        if (const int total_pages = calculate_total_pages();
            page >= 0 && page < total_pages && page != current_section_page_) {
            current_section_page_ = page;
            needs_redraw_ = true;
        }
    }

    void NavigationTUI::go_to_page(const int page) {
        if (const int total_pages = calculate_total_pages(); page >= 0 && page < total_pages && page != current_page_) {
            current_page_ = page;

            if (on_page_changed_) {
                on_page_changed_(page, total_pages);
            }

            needs_redraw_ = true;
        }
    }

    void NavigationTUI::next_page() {
        if (current_state_ == NavigationState::MAIN_MENU) {
            go_to_section_page(current_section_page_ + 1);
        } else if (current_state_ == NavigationState::ITEM_SELECTION) {
            go_to_page(current_page_ + 1);
        }
    }

    void NavigationTUI::previous_page() {
        if (current_state_ == NavigationState::MAIN_MENU) {
            go_to_section_page(current_section_page_ - 1);
        } else if (current_state_ == NavigationState::ITEM_SELECTION) {
            go_to_page(current_page_ - 1);
        }
    }

    std::map<std::string, std::vector<std::string>> NavigationTUI::get_all_selections() const {
        std::map<std::string, std::vector<std::string>> selections;
        for (const auto& section : sections_) {
            if (auto selected_items = section.get_selected_names(); !selected_items.empty()) {
                selections[section.name] = selected_items;
            }
        }
        return selections;
    }

    std::vector<std::string> NavigationTUI::get_section_selections(const size_t section_index) const {
        return (section_index < sections_.size()) ? sections_[section_index].get_selected_names()
                                                  : std::vector<std::string>{};
    }

    void NavigationTUI::clear_all_selections() {
        for (auto& section : sections_) {
            section.clear_selections();
        }
        needs_redraw_ = true;
    }

    void NavigationTUI::clear_section_selections(const size_t section_index) {
        if (section_index < sections_.size()) {
            sections_[section_index].clear_selections();
            needs_redraw_ = true;
        }
    }

    void NavigationTUI::update_config(const Config& new_config) {
        config_ = new_config;
        needs_redraw_ = true;
    }

    void NavigationTUI::update_theme(const Theme& new_theme) {
        config_.theme = new_theme;
        needs_redraw_ = true;
    }

    void NavigationTUI::update_layout(const Layout& new_layout) {
        config_.layout = new_layout;
        needs_redraw_ = true;
    }

    void NavigationTUI::update_text_config(const TextConfig& new_text_config) {
        config_.text = new_text_config;
        needs_redraw_ = true;
    }

    const NavigationTUI::Config& NavigationTUI::get_config() const { return config_; }

    void NavigationTUI::initialize() {
        terminal_manager_->setup_terminal();
        validate_indices();

        auto [t_height, t_width] = TerminalManager::get_terminal_size();
        previous_width_ = t_width;
        previous_height_ = t_height;

        needs_redraw_ = true;
    }

    int NavigationTUI::calculate_total_pages() const {
        if (current_state_ == NavigationState::MAIN_MENU) {
            return (!config_.layout.paginate_sections || sections_.empty())
                ? 1
                : (static_cast<int>((sections_.size() + config_.layout.sections_per_page - 1)) /
                   config_.layout.sections_per_page);
        }

        if (current_section_index_ < sections_.size()) {
            const size_t item_count = sections_[current_section_index_].size();
            if (item_count == 0) {
                return 1;
            }
            return static_cast<int>((item_count + config_.layout.items_per_page - 1) / config_.layout.items_per_page);
        }

        return 1;
    }

    std::pair<size_t, size_t> NavigationTUI::get_current_page_bounds() const {
        if (current_state_ != NavigationState::ITEM_SELECTION || current_section_index_ >= sections_.size()) {
            return {0, 0};
        }

        size_t start = current_page_ * config_.layout.items_per_page;
        size_t end = std::min(start + config_.layout.items_per_page, sections_[current_section_index_].size());

        return {start, end};
    }

    void NavigationTUI::clamp_selection() {
        if (current_state_ == NavigationState::MAIN_MENU && current_section_index_ >= sections_.size()) {
            current_selection_index_ = !sections_.empty() ? sections_.size() - 1 : 0;
        } else {
            auto [first, second] = get_current_page_bounds();
            if (const size_t max_selection = second - first; current_selection_index_ >= max_selection) {
                current_selection_index_ = max_selection > 0 ? max_selection - 1 : 0;
            }
        }
    }

    void NavigationTUI::change_state(const NavigationState new_state) {
        if (current_state_ != new_state) {
            const NavigationState old_state = current_state_;
            current_state_ = new_state;

            if (on_state_changed_) {
                on_state_changed_(old_state, new_state);
            }
        }
    }

    void NavigationTUI::validate_indices() {
        if (current_section_index_ >= sections_.size()) {
            current_section_index_ = !sections_.empty() ? sections_.size() - 1 : 0;
        }
        clamp_selection();
    }

} // namespace tui

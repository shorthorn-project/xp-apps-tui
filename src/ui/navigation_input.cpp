#include <cctype>
#include "core/terminal.hpp"
#include "rebuildtui.hpp"

namespace tui {

    void NavigationTUI::process_events() {
        if (auto [t_height, t_width] = TerminalManager::get_terminal_size();
            t_width != previous_width_ || t_height != previous_height_) {
            previous_width_ = t_width;
            previous_height_ = t_height;
            needs_redraw_ = true;
        }

        while (auto key_event = TerminalManager::get_key_input()) {
            handle_input(key_event->key, key_event->character);
        }
    }

    void NavigationTUI::handle_input(const Key key, const char character) {
        if (std::tolower(character) == std::tolower(config_.keys.quit_key)) {
            exit();
            return;
        }

        if (on_custom_command_ && on_custom_command_(character, current_state_)) {
            return;
        }

        handle_item_input(key, character);
    }

    void NavigationTUI::handle_item_input(const Key key, const char character) {
        switch (key) {
        case Key::ESCAPE:
            return_to_sections();
            break;

        case Key::ARROW_UP:
            move_selection_up();
            break;

        case Key::ARROW_DOWN:
            move_selection_down();
            break;

        case Key::ARROW_LEFT:
            previous_page();
            break;

        case Key::ARROW_RIGHT:
            next_page();
            break;

        case Key::SPACE:
            toggle_current_item();
            break;

        case Key::ENTER:
            // FIXME: Fucking hell...
            if (current_state_ == NavigationState::ITEM_SELECTION) {
                return_to_sections();
            } else if (current_state_ == NavigationState::MAIN_MENU) {
                select_current_item();
            }
            break;

        case Key::NORMAL:
            if (current_state_ == NavigationState::ITEM_SELECTION) {
                if (character == config_.keys.back_key) {
                    return_to_sections();
                } else if (character == config_.keys.select_all_key) {
                    if (current_section_index_ < sections_.size()) {
                        sections_[current_section_index_].select_all();
                        needs_redraw_ = true;
                    }
                } else if (character == config_.keys.select_none_key) {
                    if (current_section_index_ < sections_.size()) {
                        sections_[current_section_index_].clear_selections();
                        needs_redraw_ = true;
                    }
                }
            } else if (current_state_ == NavigationState::MAIN_MENU && std::isdigit(character)) {
                handle_number_input(character);
            }
            break;

        default:
            if (config_.enable_vim_keys) {
                if (character == 'j') {
                    move_selection_down();
                } else if (character == 'k') {
                    move_selection_up();
                } else if (character == config_.keys.back_key) {
                    return_to_sections();
                }
            }
            break;
        }
    }

    void NavigationTUI::move_selection_up() {
        if (current_state_ == NavigationState::MAIN_MENU) {
            if (current_selection_index_ > 0) {
                current_selection_index_--;
            } else if (current_section_page_ > 0) {
                go_to_section_page(current_section_page_ - 1);
                current_selection_index_ = get_sections_on_current_page() - 1;
            }
        } else {
            if (current_selection_index_ > 0) {
                current_selection_index_--;
            } else {
                if (current_page_ > 0) {
                    go_to_page(current_page_ - 1);
                    auto [first, second] = get_current_page_bounds();
                    current_selection_index_ = second - first - 1;
                }
            }
        }
        needs_redraw_ = true;
    }

    void NavigationTUI::move_selection_down() {
        if (current_state_ == NavigationState::MAIN_MENU) {
            const int items_on_page = get_sections_on_current_page();
            const int total_pages = calculate_total_pages();

            if (static_cast<int>(current_selection_index_) < items_on_page - 1) {
                current_selection_index_++;
            } else if (current_section_page_ < total_pages - 1) {
                go_to_section_page(current_section_page_ + 1);
                current_selection_index_ = 0;
            }
        } else {
            auto [first, second] = get_current_page_bounds();

            if (const size_t items_on_page = second - first; current_selection_index_ < items_on_page - 1) {
                current_selection_index_++;
            } else {
                if (const int total_pages = calculate_total_pages(); current_page_ < total_pages - 1) {
                    go_to_page(current_page_ + 1);
                    current_selection_index_ = 0;
                }
            }
        }
        needs_redraw_ = true;
    }

    void NavigationTUI::select_current_item() {
        if (current_state_ == NavigationState::MAIN_MENU) {
            size_t global_index = current_section_page_ * config_.layout.sections_per_page + current_selection_index_;
            if (global_index < sections_.size()) {
                enter_section(global_index);
            }
        } else {
            toggle_current_item();
        }
    }

    void NavigationTUI::toggle_current_item() {
        if (current_state_ == NavigationState::ITEM_SELECTION && current_section_index_ < sections_.size()) {
            auto [start, end] = get_current_page_bounds();
            if (const size_t global_index = start + current_selection_index_;
                sections_[current_section_index_].toggle_item(global_index)) {
                if (on_item_toggled_) {
                    if (const auto* item = sections_[current_section_index_].get_item(global_index)) {
                        on_item_toggled_(current_section_index_, global_index, item->selected);
                    }
                }
                needs_redraw_ = true;
            }
        }
    }

    void NavigationTUI::handle_number_input(const char digit) {
        const int number = digit - '0';

        if (current_state_ == NavigationState::MAIN_MENU) {
            if (number > 0 && number <= static_cast<int>(sections_.size())) {
                const auto global_index = number - 1;
                current_section_page_ = global_index / config_.layout.sections_per_page;
                current_selection_index_ = global_index % config_.layout.sections_per_page;
                enter_section(global_index);
            } else if (config_.layout.paginate_sections && number > 0 && number <= calculate_total_pages()) {
                go_to_section_page(number - 1);
            }
        } else if (current_state_ == NavigationState::ITEM_SELECTION && number > 0) {
            go_to_page(number - 1);
        }
    }

} // namespace tui

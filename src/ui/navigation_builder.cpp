#include "rebuildtui.hpp"

namespace tui {

    NavigationBuilder& NavigationBuilder::theme_indicators(const char selected, const char unselected) {
        config_.theme.selected_indicator = selected;
        config_.theme.unselected_indicator = unselected;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::theme_prefixes(const std::string& selected, const std::string& unselected,
                                                         const std::string& highlighted) {
        config_.theme.selected_prefix = selected;
        config_.theme.unselected_prefix = unselected;
        config_.theme.highlighted_prefix = highlighted;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::theme_unicode(const bool enable) {
        config_.theme.use_unicode = enable;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::theme_colors(const bool enable) {
        config_.theme.use_colors = enable;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::theme_gradient_support(const bool enable) {
        config_.theme.gradient_enabled = enable;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::theme_gradient_preset(const extras::GradientPreset& preset) {
        config_.theme.gradient_preset = preset;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::theme_gradient_randomize(const bool enable) {
        config_.theme.gradient_randomize = enable;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::theme_border_style(const extras::BorderStyle& style) {
        config_.theme.border_style = style;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::theme_accent_color(const extras::AccentColor& color) {
        config_.theme.accent_color = color;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::theme_palette(const extras::ColorPalette& palette) {
        config_.theme.palette = palette;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::theme_color(const std::string& element, const extras::Color& color) {
        if (element == "border") {
            config_.theme.palette.border = color;
        } else if (element == "header_text") {
            config_.theme.palette.header_text = color;
        } else if (element == "header_border") {
            config_.theme.palette.header_border = color;
        } else if (element == "section_name") {
            config_.theme.palette.section_name = color;
        } else if (element == "item_name") {
            config_.theme.palette.item_name = color;
        } else if (element == "selected_item") {
            config_.theme.palette.selected_item = color;
        } else if (element == "unselected_item") {
            config_.theme.palette.unselected_item = color;
        } else if (element == "counter") {
            config_.theme.palette.counter = color;
        } else if (element == "footer") {
            config_.theme.palette.footer = color;
        }
        return *this;
    }

    NavigationBuilder& NavigationBuilder::layout_centering(const bool horizontal, const bool vertical) {
        config_.layout.center_horizontally = horizontal;
        config_.layout.center_vertically = vertical;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::layout_content_width(const int min_width, const int max_width) {
        config_.layout.min_content_width = min_width;
        config_.layout.max_content_width = max_width;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::layout_padding(const int vertical_padding) {
        config_.layout.vertical_padding = vertical_padding;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::layout_auto_resize(const bool enable) {
        config_.layout.auto_resize_content = enable;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::layout_borders(const bool show) {
        config_.layout.show_borders = show;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::layout_items_per_page(const int count) {
        config_.layout.items_per_page = count;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::layout_sections_per_page(const int count) {
        config_.layout.sections_per_page = count;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::paginate_sections(const bool paginate) {
        config_.layout.paginate_sections = paginate;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::text_titles(const std::string& section_title,
                                                      const std::string& item_prefix) {
        config_.text.section_selection_title = section_title;
        config_.text.item_selection_prefix = item_prefix;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::text_messages(const std::string& empty_message) {
        config_.text.empty_section_message = empty_message;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::text_help(const std::string& section_help, const std::string& item_help) {
        config_.text.help_text_sections = section_help;
        config_.text.help_text_items = item_help;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::text_show_help(const bool show) {
        config_.text.show_help_text = show;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::text_show_pages(const bool show) {
        config_.text.show_page_numbers = show;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::text_show_counters(const bool show) {
        config_.text.show_counters = show;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::keys_quick_select(const bool enable) {
        config_.enable_quick_select = enable;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::keys_vim_style(const bool enable) {
        config_.enable_vim_keys = enable;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::keys_custom_shortcut(const char key, const std::string& description) {
        config_.custom_shortcuts[key] = description;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::add_section(const Section& section) {
        sections_.push_back(section);
        return *this;
    }

    NavigationBuilder& NavigationBuilder::add_section(Section&& section) {
        sections_.push_back(std::move(section));
        return *this;
    }

    NavigationBuilder& NavigationBuilder::add_sections(const std::vector<Section>& sections) {
        sections_.insert(sections_.end(), sections.begin(), sections.end());
        return *this;
    }

    NavigationBuilder& NavigationBuilder::on_section_selected(NavigationTUI::SectionSelectedCallback callback) {
        section_selected_callback_ = std::move(callback);
        return *this;
    }

    NavigationBuilder& NavigationBuilder::on_item_toggled(NavigationTUI::ItemToggledCallback callback) {
        item_toggled_callback_ = std::move(callback);
        return *this;
    }

    NavigationBuilder& NavigationBuilder::on_page_changed(NavigationTUI::PageChangedCallback callback) {
        page_changed_callback_ = std::move(callback);
        return *this;
    }

    NavigationBuilder& NavigationBuilder::on_state_changed(NavigationTUI::StateChangedCallback callback) {
        state_changed_callback_ = std::move(callback);
        return *this;
    }

    NavigationBuilder& NavigationBuilder::on_exit(NavigationTUI::ExitCallback callback) {
        exit_callback_ = std::move(callback);
        return *this;
    }

    NavigationBuilder& NavigationBuilder::on_custom_command(NavigationTUI::CustomCommandCallback callback) {
        custom_command_callback_ = std::move(callback);
        return *this;
    }

    NavigationBuilder& NavigationBuilder::on_update(NavigationTUI::UpdateCallback callback) {
        update_callback_ = std::move(callback);
        return *this;
    }

    NavigationBuilder& NavigationBuilder::theme_minimal() {
        config_.theme.use_unicode = false;
        config_.theme.use_colors = false;
        config_.theme.selected_prefix = "* ";
        config_.theme.unselected_prefix = "  ";
        config_.theme.border_style = extras::BorderStyle::ASCII;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::theme_fancy() {
        config_.theme.use_unicode = true;
        config_.theme.use_colors = true;
        config_.theme.selected_prefix = "✓ ";
        config_.theme.unselected_prefix = "○ ";
        config_.theme.border_style = extras::BorderStyle::ROUNDED;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::theme_retro() {
        config_.theme.use_unicode = false;
        config_.theme.use_colors = false;
        config_.theme.selected_prefix = "[X] ";
        config_.theme.unselected_prefix = "[ ] ";
        config_.theme.border_style = extras::BorderStyle::DOUBLE;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::theme_modern() {
        config_.theme.use_unicode = true;
        config_.theme.use_colors = true;
        config_.theme.selected_prefix = "● ";
        config_.theme.unselected_prefix = "○ ";
        config_.theme.border_style = extras::BorderStyle::ROUNDED;
        config_.theme.accent_color = extras::AccentColor::BLUE;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::layout_compact() {
        config_.layout.items_per_page = 25;
        config_.layout.show_borders = false;
        config_.layout.center_horizontally = false;
        config_.layout.center_vertically = false;
        config_.layout.min_content_width = 40;
        config_.layout.max_content_width = 60;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::layout_comfortable() {
        config_.layout.items_per_page = 15;
        config_.layout.show_borders = true;
        config_.layout.center_horizontally = false;
        config_.layout.center_vertically = false;
        config_.layout.min_content_width = 60;
        config_.layout.max_content_width = 100;
        config_.layout.vertical_padding = 2;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::layout_fullscreen() {
        config_.layout.items_per_page = 30;
        config_.layout.show_borders = true;
        config_.layout.center_horizontally = false;
        config_.layout.center_vertically = false;
        config_.layout.auto_resize_content = true;
        config_.layout.min_content_width = 80;
        config_.layout.max_content_width = 120;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::layout_centered() {
        config_.layout.center_horizontally = true;
        config_.layout.center_vertically = false;
        config_.layout.items_per_page = 20;
        config_.layout.show_borders = true;
        config_.layout.min_content_width = 60;
        config_.layout.max_content_width = 80;
        config_.layout.vertical_padding = 3;
        return *this;
    }

    std::unique_ptr<NavigationTUI> NavigationBuilder::build() {
        auto tui = std::make_unique<NavigationTUI>(config_);

        for (auto& section : sections_) {
            tui->add_section(std::move(section));
        }

        if (section_selected_callback_) {
            tui->set_section_selected_callback(section_selected_callback_);
        }
        if (item_toggled_callback_) {
            tui->set_item_toggled_callback(item_toggled_callback_);
        }
        if (page_changed_callback_) {
            tui->set_page_changed_callback(page_changed_callback_);
        }
        if (state_changed_callback_) {
            tui->set_state_changed_callback(state_changed_callback_);
        }
        if (exit_callback_) {
            tui->set_exit_callback(exit_callback_);
        }
        if (custom_command_callback_) {
            tui->set_custom_command_callback(custom_command_callback_);
        }
        if (update_callback_) {
            tui->set_update_callback(update_callback_);
        }

        return tui;
    }

    const NavigationTUI::Config& NavigationBuilder::get_config() const { return config_; }

    NavigationBuilder& NavigationBuilder::reset() {
        config_ = NavigationTUI::Config{};
        sections_.clear();
        section_selected_callback_ = nullptr;
        item_toggled_callback_ = nullptr;
        page_changed_callback_ = nullptr;
        state_changed_callback_ = nullptr;
        exit_callback_ = nullptr;
        custom_command_callback_ = nullptr;
        update_callback_ = nullptr;
        return *this;
    }

} // namespace tui

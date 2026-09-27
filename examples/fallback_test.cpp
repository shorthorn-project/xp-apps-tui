#include <fmt/core.h>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "rebuildtui.hpp"
#include "ui/section_builder.hpp"

using namespace tui;

int main() {
    try {
        auto security = SectionBuilder("🛡️ Security & Privacy")
                            .description("Configure system defensive services and data collection policies")
                            .add_item("Telemetry Service", "Disable diagnostic data transmission to remote servers")
                            .add_item("Local Security Authority protection",
                                      "Enable additional LSA protection to prevent credential dumping")
                            .add_item("Location Platform Access", "Disable core OS location services and sensors")
                            .select_items({"Telemetry Service"})
                            .build();

        auto performance =
            SectionBuilder("⚙️ Performance Tweaks")
                .description("Adjust processor, memory, and paging configurations")
                .add_item("Optimize Processor Scheduling", "Set execution priority scheduling to Background Services")
                .add_item("Core System Paging",
                          "Disable executive paging to force system code to remain in physical memory")
                .add_item("Graphics Hardware Acceleration", "Enable hardware-accelerated GPU scheduling")
                .build();

        auto network =
            SectionBuilder("🔌 Network Configuration")
                .description("Tune low-level network adapter and protocol parameters")
                .add_item("Secure DNS Resolution", "Force encrypted DNS-over-HTTPS resolution")
                .add_item("Adapter Power Throttling", "Prevent OS from disabling network adapter to save power")
                .build();

        auto tui = NavigationBuilder()
                       .text_titles("💻 System Configuration Tool", "⚙️ Select Section: ")
                       .text_help("Up/Down: Navigate | Enter: Select | Q: Quit",
                                  "Up/Down: Navigate | Space: Toggle | Enter: Back")
                       .theme_unicode(true)
                       .theme_prefixes("✅", "❌")
                       .layout_centered()
                       .add_sections({security, performance, network})
                       .on_exit([](const std::vector<Section>& secs) {
                           std::cout << "\ntest\n";
                           std::cout << "====================\n";
                           for (const auto& section : secs) {
                               auto selected = section.get_selected_names();
                               if (!selected.empty()) {
                                   std::cout << "Section: " << section.name << "\n";
                                   for (const auto& item : selected) {
                                       std::cout << "  - " << item << "\n";
                                   }
                               }
                           }
                           std::cout << "\nPress Enter to exit...";
                           std::cin.get();
                       })
                       .build();

        tui->run();
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Unknown error occurred\n";
        return 1;
    }
}

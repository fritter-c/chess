#include "profiler_window.hpp"
#include <cstdint>
#include "imgui.h"
#include "profiler.hpp"
gtr::profiler::profiler gtr::profiler::GlobalProfiler;
uint32_t gtr::profiler::GlobalProfilerParent;
namespace renderer {
void render_profiler() {
    ImGui::Begin("Profiler", nullptr);
    ImGui::Text("Profiler");
    ImGui::SameLine();
    if (ImGui::Button("Reset")) {
        gtr::profiler::Reset();
    }
    ImGui::Separator();
    ImGui::TextUnformatted(gtr::profiler::EndAndPrintProfile());
    ImGui::End();
}
} // namespace renderer
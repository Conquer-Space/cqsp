#include "cqspgui.h"

//namespace CQSPGui
conquerspace::engine::Application* AppContext = nullptr;
void CQSPGui::SetApplication(conquerspace::engine::Application* context) {
    AppContext = context;
}

bool CQSPGui::DefaultButton(const char* name, const ImVec2& size) {
    if (ImGui::Button(name, size)) {
        AppContext->GetAudioInterface().PlayAudioClip("button.press");
        return true;
    }
    return false;
}

bool CQSPGui::SmallDefaultButton(const char* label) {
    if (ImGui::SmallButton(label)) {
        AppContext->GetAudioInterface().PlayAudioClip("button.press");
        return true;
    }
    return false;
}

bool CQSPGui::DefaultSelectable(const char* label,
                                bool selected, ImGuiSelectableFlags flags, const ImVec2& size) {
    if (ImGui::Selectable(label, selected, flags, size)) {
        AppContext->GetAudioInterface().PlayAudioClip("button.press");
        return true;
    }
    return false;
}

bool CQSPGui::DefaultSelectable(const char* label,
                            bool* p_selected, ImGuiSelectableFlags flags, const ImVec2& size) {
    if (ImGui::Selectable(label, p_selected, flags, size)) {
        AppContext->GetAudioInterface().PlayAudioClip("button.press");
        return true;
    }
    return false;
}

bool CQSPGui::DefaultCheckbox(const char* label, bool* v) {
    if (ImGui::Checkbox(label, v)) {
        AppContext->GetAudioInterface().PlayAudioClip("button.press");
        return true;
    }
    return false;
}

bool CQSPGui::ArrowButton(const char* label, ImGuiDir dir) {
    if (ImGui::ArrowButton(label, dir)) {
        AppContext->GetAudioInterface().PlayAudioClip("button.back");
        return true;
    }
    return false;
}

bool CQSPGui::SliderFloat(const char* label, float* v, float v_min, float v_max,
                          const char* format, ImGuiSliderFlags flags) {
    if (ImGui::SliderFloat(label, v, v_min, v_max, format, flags)) {
        AppContext->GetAudioInterface().PlayAudioClip("scroll.tick");
        return true;
    }
    return false;
}

bool CQSPGui::SliderInt(const char* label, int* v, int v_min, int v_max,
                        const char* format, ImGuiSliderFlags flags) {
    if (ImGui::SliderInt(label, v, v_min, v_max, format, flags)) {
        AppContext->GetAudioInterface().PlayAudioClip("scroll.tick");
        return true;
    }
    return false;
}

bool CQSPGui::DragFloat(const char* label, float* v, float v_speed, float v_min,
                        float v_max, const char* format,
                        ImGuiSliderFlags flags) {
    if (ImGui::DragFloat(label, v, v_speed, v_min, v_max, format, flags)) {
        AppContext->GetAudioInterface().PlayAudioClip("scroll.tick");
        return true;
    }
    return false;
}

bool CQSPGui::DragInt(const char* label, int* v, float v_speed, int v_min,
                      int v_max, const char* format, ImGuiSliderFlags flags) {
    if (ImGui::DragInt(label, v, v_speed, v_min, v_max, format, flags)) {
        AppContext->GetAudioInterface().PlayAudioClip("scroll.tick");
        return true;
    }
    return false;
}


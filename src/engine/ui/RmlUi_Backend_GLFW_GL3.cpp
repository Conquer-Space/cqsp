/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <GLFW/glfw3.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Input.h>
#include <RmlUi/Core/Profiling.h>

#include "RmlUi_Backend.h"
#include "RmlUi_Platform_GLFW.h"
#include "RmlUi_Renderer_GL3.h"

static void SetupCallbacks(GLFWwindow* window);

static void LogErrorFromGLFW(int error, const char* description) {
    Rml::Log::Message(Rml::Log::LT_ERROR, "GLFW error (0x%x): %s", error, description);
}

/**
    Global data used by this backend.

    Lifetime governed by the calls to Backend::Initialize() and Backend::Shutdown().
 */
struct BackendData {
    SystemInterface_GLFW system_interface;
    RenderInterface_GL3 render_interface;
    GLFWwindow* window = nullptr;
    int glfw_active_modifiers = 0;
    bool context_dimensions_dirty = true;

    // Arguments set during event processing and nulled otherwise.
    Rml::Context* context = nullptr;
    KeyDownCallback key_down_callback = nullptr;
};
static Rml::UniquePtr<BackendData> data;

bool Backend::Initialize(const char* name, int width, int height, bool allow_resize) { return true; }

void Backend::Shutdown() {
    RMLUI_ASSERT(data);
    glfwDestroyWindow(data->window);
    data.reset();
    RmlGL3::Shutdown();
    glfwTerminate();
}

Rml::SystemInterface* Backend::GetSystemInterface() {
    RMLUI_ASSERT(data);
    return &data->system_interface;
}

Rml::RenderInterface* Backend::GetRenderInterface() {
    RMLUI_ASSERT(data);
    return &data->render_interface;
}

bool Backend::ProcessEvents(Rml::Context* context, KeyDownCallback key_down_callback, bool power_save) {
    RMLUI_ASSERT(data && context);

    // The initial window size may have been affected by system DPI settings, apply the actual pixel size and dp-ratio to the context.
    if (data->context_dimensions_dirty) {
        data->context_dimensions_dirty = false;

        Rml::Vector2i window_size;
        float dp_ratio = 1.f;
        glfwGetFramebufferSize(data->window, &window_size.x, &window_size.y);
        glfwGetWindowContentScale(data->window, &dp_ratio, nullptr);

        context->SetDimensions(window_size);
        context->SetDensityIndependentPixelRatio(dp_ratio);
    }

    data->context = context;
    data->key_down_callback = key_down_callback;

    if (power_save)
        glfwWaitEventsTimeout(Rml::Math::Min(context->GetNextUpdateDelay(), 10.0));
    else
        glfwPollEvents();

    data->context = nullptr;
    data->key_down_callback = nullptr;

    const bool result = !glfwWindowShouldClose(data->window);
    glfwSetWindowShouldClose(data->window, GLFW_FALSE);
    return result;
}

void Backend::RequestExit() {
    RMLUI_ASSERT(data);
    glfwSetWindowShouldClose(data->window, GLFW_TRUE);
}

void Backend::BeginFrame() {
    RMLUI_ASSERT(data);
    data->render_interface.Clear();
    data->render_interface.BeginFrame();
}

void Backend::PresentFrame() {
    RMLUI_ASSERT(data);
    data->render_interface.EndFrame();
    glfwSwapBuffers(data->window);

    // Optional, used to mark frames during performance profiling.
    RMLUI_FrameMark;
}

static void SetupCallbacks(GLFWwindow* window) {
    RMLUI_ASSERT(data);

    // Key input
    glfwSetKeyCallback(
        window, [](GLFWwindow* /*window*/, int glfw_key, int /*scancode*/, int glfw_action, int glfw_mods) {
            if (!data->context) return;

            // Store the active modifiers for later because GLFW doesn't provide them in the callbacks to the mouse input events.
            data->glfw_active_modifiers = glfw_mods;

            // Override the default key event callback to add global shortcuts for the samples.
            Rml::Context* context = data->context;
            KeyDownCallback key_down_callback = data->key_down_callback;

            switch (glfw_action) {
                case GLFW_PRESS:
                case GLFW_REPEAT: {
                    const Rml::Input::KeyIdentifier key = RmlGLFW::ConvertKey(glfw_key);
                    const int key_modifier = RmlGLFW::ConvertKeyModifiers(glfw_mods);
                    float dp_ratio = 1.f;
                    glfwGetWindowContentScale(data->window, &dp_ratio, nullptr);

                    // See if we have any global shortcuts that take priority over the context.
                    if (key_down_callback && !key_down_callback(context, key, key_modifier, dp_ratio, true)) break;
                    // Otherwise, hand the event over to the context by calling the input handler as normal.
                    if (!RmlGLFW::ProcessKeyCallback(context, glfw_key, glfw_action, glfw_mods)) break;
                    // The key was not consumed by the context either, try keyboard shortcuts of lower priority.
                    if (key_down_callback && !key_down_callback(context, key, key_modifier, dp_ratio, false)) break;
                } break;
                case GLFW_RELEASE:
                    RmlGLFW::ProcessKeyCallback(context, glfw_key, glfw_action, glfw_mods);
                    break;
            }
        });

    glfwSetCharCallback(window, [](GLFWwindow* /*window*/, unsigned int codepoint) {
        RmlGLFW::ProcessCharCallback(data->context, codepoint);
    });

    glfwSetCursorEnterCallback(window, [](GLFWwindow* /*window*/, int entered) {
        RmlGLFW::ProcessCursorEnterCallback(data->context, entered);
    });

    // Mouse input
    glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xpos, double ypos) {
        RmlGLFW::ProcessCursorPosCallback(data->context, window, xpos, ypos, data->glfw_active_modifiers);
    });

    glfwSetMouseButtonCallback(window, [](GLFWwindow* /*window*/, int button, int action, int mods) {
        data->glfw_active_modifiers = mods;
        RmlGLFW::ProcessMouseButtonCallback(data->context, button, action, mods);
    });

    glfwSetScrollCallback(window, [](GLFWwindow* /*window*/, double /*xoffset*/, double yoffset) {
        RmlGLFW::ProcessScrollCallback(data->context, yoffset, data->glfw_active_modifiers);
    });

    // Window events
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* /*window*/, int width, int height) {
        data->render_interface.SetViewport(width, height);
        RmlGLFW::ProcessFramebufferSizeCallback(data->context, width, height);
    });

    glfwSetWindowContentScaleCallback(window, [](GLFWwindow* /*window*/, float xscale, float /*yscale*/) {
        RmlGLFW::ProcessContentScaleCallback(data->context, xscale);
    });
}

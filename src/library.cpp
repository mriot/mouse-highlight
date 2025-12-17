#include <library.h>
#include <Version.h>
#include <chrono>
#include <filesystem>
#include <format>
#include <mumble/Mumble.h>
#include <nexus/Nexus.h>
#include <inifile/inicpp.h>
#include <imgui/imgui.h>


/// ----------------------------------------------------------------------------------------------------
/// TYPES
/// ----------------------------------------------------------------------------------------------------

enum class CrosshairMode : int
{
    Always = 0,
    InCombat = 1,
    NotInCombat = 2
};


/// ----------------------------------------------------------------------------------------------------
/// GLOBALS
/// ----------------------------------------------------------------------------------------------------

namespace G
{
    constexpr auto ADDON_NAME = "Mouse Highlight";

    AddonAPI_t *APIDefs        = nullptr;
    NexusLinkData_t *NexusLink = nullptr;
    Mumble::Data *MumbleLink   = nullptr;
}


/// ----------------------------------------------------------------------------------------------------
/// LOGGING HELPERS
/// ----------------------------------------------------------------------------------------------------

namespace Log
{
    inline void Debug(std::string_view msg) { G::APIDefs->Log(LOGL_DEBUG, G::ADDON_NAME, msg.data()); }
    inline void Info(std::string_view msg) { G::APIDefs->Log(LOGL_INFO, G::ADDON_NAME, msg.data()); }
    inline void Warn(std::string_view msg) { G::APIDefs->Log(LOGL_WARNING, G::ADDON_NAME, msg.data()); }
    inline void Crit(std::string_view msg) { G::APIDefs->Log(LOGL_CRITICAL, G::ADDON_NAME, msg.data()); }
}


/// ----------------------------------------------------------------------------------------------------
/// CROSSHAIR
/// ----------------------------------------------------------------------------------------------------

namespace Crosshair
{
    bool IsCursorVisible()
    {
        CURSORINFO ci = {sizeof(ci)};
        if (!GetCursorInfo(&ci))
            return false;
        return (ci.flags & CURSOR_SHOWING) != 0;
    }

    void RenderCrosshair(const ImVec4 &color, const float thickness, const bool showWhenCursorHidden, const bool renderOnTop)
    {
        ImDrawList *drawList   = renderOnTop ? ImGui::GetForegroundDrawList() : ImGui::GetBackgroundDrawList();
        const ImVec2 cursorPos = ImGui::GetIO().MousePos;
        const ImVec2 screenDim = ImGui::GetIO().DisplaySize;

        const ImVec2 pointsX[] = {
            ImVec2(cursorPos.x, 0),
            ImVec2(cursorPos.x, screenDim.y)
        };

        const ImVec2 pointsY[] = {
            ImVec2(0, cursorPos.y),
            ImVec2(screenDim.x, cursorPos.y)
        };

        if (showWhenCursorHidden || IsCursorVisible())
        {
            drawList->AddPolyline(pointsX, 2, ImGui::ColorConvertFloat4ToU32(color), false, thickness);
            drawList->AddPolyline(pointsY, 2, ImGui::ColorConvertFloat4ToU32(color), false, thickness);
        }
    }
}


/// ----------------------------------------------------------------------------------------------------
/// SETTINGS
/// ----------------------------------------------------------------------------------------------------


namespace Settings
{
    ini::IniFile config;
    std::filesystem::path settingsPath;

    template <typename T>
    T GetIniValueOrDefault(const std::string &section, const std::string &key, const T &defaultValue)
    {
        try
        {
            if (!std::filesystem::exists(settingsPath))
                return defaultValue;
            return config[section][key].as<T>();
        }
        catch (const std::exception &)
        {
            return defaultValue;
        }
    }

    void Init()
    {
        try
        {
            settingsPath = G::APIDefs->Paths_GetAddonDirectory("MouseHighlight");
            std::filesystem::create_directories(settingsPath);
            settingsPath /= "settings.ini";

            config.load(settingsPath.string());
        }
        catch (const std::exception &e)
        {
            Log::Crit(std::format("Failed to load settings: {}", e.what()));
        }
    }

    CrosshairMode GetCrosshairPreference()
    {
        const int style = GetIniValueOrDefault<int>("MouseHighlight", "crosshair", (int)CrosshairMode::Always);
        return style >= 0 && style <= 2 ? static_cast<CrosshairMode>(style) : CrosshairMode::Always;
    }

    void SetCrosshairPreference(const CrosshairMode style)
    {
        config["MouseHighlight"]["crosshair"] = static_cast<int>(style);
        config.save(settingsPath.string());
    }

    bool GetCrosshairShowWhenHiddenPreference()
    {
        const bool showWhenHidden = GetIniValueOrDefault("MouseHighlight", "show_when_cursor_hidden", false);
        return showWhenHidden;
    }

    void SetCrosshairShowWhenHiddenPreference(const bool showWhenHidden)
    {
        config["MouseHighlight"]["show_when_cursor_hidden"] = showWhenHidden;
        config.save(settingsPath.string());
    }

    bool GetCrosshairRenderOnTopPreference()
    {
        const bool renderOnTop = GetIniValueOrDefault("MouseHighlight", "render_on_top", false);
        return renderOnTop;
    }

    void SetCrosshairRenderOnTopPreference(const bool renderOnTop)
    {
        config["MouseHighlight"]["render_on_top"] = renderOnTop;
        config.save(settingsPath.string());
    }

    float GetCrosshairThicknessPreference()
    {
        const float thickness = GetIniValueOrDefault("MouseHighlight", "thickness", 1.0f);
        return thickness;
    }

    void SetCrosshairThicknessPreference(const float thickness)
    {
        config["MouseHighlight"]["thickness"] = thickness;
        config.save(settingsPath.string());
    }

    ImVec4 GetCrosshairColorPreference()
    {
        ImVec4 color;

        color.x = GetIniValueOrDefault("MouseHighlight", "color_r", 1.0f);
        color.y = GetIniValueOrDefault("MouseHighlight", "color_g", 0.0f);
        color.z = GetIniValueOrDefault("MouseHighlight", "color_b", 0.0f);
        color.w = GetIniValueOrDefault("MouseHighlight", "color_a", 1.0f);

        return color;
    }

    void SetCrosshairColorPreference(const ImVec4 &color)
    {
        config["MouseHighlight"]["color_r"] = color.x;
        config["MouseHighlight"]["color_g"] = color.y;
        config["MouseHighlight"]["color_b"] = color.z;
        config["MouseHighlight"]["color_a"] = color.w;
        config.save(settingsPath.string());
    }

    void OnOptionsRender()
    {
        static CrosshairMode s_crosshairStyle = GetCrosshairPreference();
        static float s_crosshairThickness     = GetCrosshairThicknessPreference();
        static ImVec4 s_crosshairColor        = GetCrosshairColorPreference();
        static bool s_showWhenCursorHidden    = GetCrosshairShowWhenHiddenPreference();
        static bool s_renderOnTop             = GetCrosshairRenderOnTopPreference();

        const char *items[] = {"Always", "In combat", "Not in combat"};

        ImGui::TextUnformatted("Show crosshair:");
        for (int i = 0; i < IM_ARRAYSIZE(items); ++i)
        {
            if (ImGui::RadioButton(items[i], s_crosshairStyle == static_cast<CrosshairMode>(i)))
            {
                s_crosshairStyle = static_cast<CrosshairMode>(i);
                SetCrosshairPreference(s_crosshairStyle);
            }
        }

        if (ImGui::Checkbox("Also show when cursor is hidden", &s_showWhenCursorHidden))
            SetCrosshairShowWhenHiddenPreference(s_showWhenCursorHidden);

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Might not work so well with ActionCam");

        if (ImGui::Checkbox("Render on top of ImGui windows", &s_renderOnTop))
            SetCrosshairRenderOnTopPreference(s_renderOnTop);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::ColorButton("Right click to edit", s_crosshairColor);
        if (ImGui::BeginPopupContextItem("ColorPopup"))
        {
            ImGui::ColorPicker4("##picker", (float *)&s_crosshairColor, ImGuiColorEditFlags_AlphaBar);
            ImGui::EndPopup();
            SetCrosshairColorPreference(s_crosshairColor);
        }
        ImGui::SameLine();
        ImGui::Text("Crosshair color");

        ImGui::Spacing();

        ImGui::SetNextItemWidth(250.0f);
        if (ImGui::SliderFloat("Thickness", &s_crosshairThickness, 0.1f, 10.0f, "%.1f px"))
            SetCrosshairThicknessPreference(s_crosshairThickness);
    }
}


/// ----------------------------------------------------------------------------------------------------
/// RUNTIME
/// ----------------------------------------------------------------------------------------------------

namespace Runtime
{
    void OnRender()
    {
        if (!G::NexusLink->IsGameplay)
            return;

        const bool inCombat       = G::MumbleLink->Context.IsInCombat;
        const ImVec4 color        = Settings::GetCrosshairColorPreference();
        const float thickness     = Settings::GetCrosshairThicknessPreference();
        const CrosshairMode mode  = Settings::GetCrosshairPreference();
        const bool showWhenHidden = Settings::GetCrosshairShowWhenHiddenPreference();
        const bool renderOnTop    = Settings::GetCrosshairRenderOnTopPreference();

        if (mode == CrosshairMode::Always || mode == CrosshairMode::InCombat && inCombat || mode == CrosshairMode::NotInCombat && !inCombat)
            Crosshair::RenderCrosshair(color, thickness, showWhenHidden, renderOnTop);
    }
}


/// ----------------------------------------------------------------------------------------------------
/// ADDON LIFECYCLE
/// ----------------------------------------------------------------------------------------------------

static void AddonLoad(AddonAPI_t *api)
{
    G::APIDefs    = api;
    G::NexusLink  = static_cast<NexusLinkData_t *>(api->DataLink_Get(DL_NEXUS_LINK));
    G::MumbleLink = static_cast<Mumble::Data *>(api->DataLink_Get(DL_MUMBLE_LINK));

    ImGui::SetCurrentContext((ImGuiContext *)G::APIDefs->ImguiContext);
    ImGui::SetAllocatorFunctions((void *(*)(size_t, void *))G::APIDefs->ImguiMalloc,
                                 (void (*)(void *, void *))G::APIDefs->ImguiFree);

    Settings::Init();

    api->GUI_Register(RT_Render, Runtime::OnRender);
    api->GUI_Register(RT_OptionsRender, Settings::OnOptionsRender);
}

static void AddonUnload()
{
    G::APIDefs->GUI_Deregister(Runtime::OnRender);
    G::APIDefs->GUI_Deregister(Settings::OnOptionsRender);
}


/// ----------------------------------------------------------------------------------------------------
/// ADDON DEFINITION
/// ----------------------------------------------------------------------------------------------------

extern "C" __declspec(dllexport) AddonDefinition_t *GetAddonDef()
{
    static AddonDefinition_t def = {
        .Signature = static_cast<uint32_t>(0x0F575D36),
        .APIVersion = NEXUS_API_VERSION,
        .Name = G::ADDON_NAME,
        .Version = {V_MAJOR, V_MINOR, V_BUILD, V_REVISION},
        .Author = "Eredin.4187",
        .Description = "TacO addon port that renders a full-screen crosshair aligned to the mouse cursor.",
        .Load = AddonLoad,
        .Unload = AddonUnload,
        .Provider = UP_GitHub,
        .UpdateLink = "https://github.com/mriot/mouse-highlight"
    };
    return &def;
}

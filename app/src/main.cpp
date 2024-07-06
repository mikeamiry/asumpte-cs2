#include <iostream>
#include <Windows.h>
#include <thread>
#include <TlHelp32.h>
#include <vector>
#include <Psapi.h>
#include <string>
#include <tchar.h>
#include <cstdio>

#include <dwmapi.h>
#include <d3d11.h>
#include <windowsx.h>
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"
#include "vector.h"
#include "render.h"
#include "font.h"

#include "offsets.hpp"
#include "client.dll.hpp"
#include "buttons.hpp"

#include "driver.hpp"
#include "memory.hpp"

#include "settings.hpp"

#include <math.h>
#include "ImGui/imgui_internal.h"

#include <vector>

int screenWidth = GetSystemMetrics(SM_CXSCREEN);
int screenHeight = GetSystemMetrics(SM_CYSCREEN);

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK window_procedure(HWND window, UINT message, WPARAM w_param, LPARAM l_param) {
    if (ImGui_ImplWin32_WndProcHandler(window, message, w_param, l_param)) {
        return 0L;
    }

    if (message == WM_DESTROY) {
        PostQuitMessage(0);
        return 0L;
    }

    switch (message)
    {
    case WM_NCHITTEST:
    {
        const LONG borderWidth = GetSystemMetrics(SM_CXSIZEFRAME);
        const LONG titleBarHeight = GetSystemMetrics(SM_CYCAPTION);
        POINT cursorPos = { GET_X_LPARAM(w_param), GET_Y_LPARAM(l_param) };
        RECT windowRect;
        GetWindowRect(window, &windowRect);

        if (cursorPos.y >= windowRect.top && cursorPos.y < windowRect.top + titleBarHeight)
            return HTCAPTION;

        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(window, message, w_param, l_param);
}

int main() {

    int pid;
    do
    {
        system("cls");
        pid = memory::get_process_id(L"cs2.exe");
        if (pid == 0)
        {
            std::cout << "Waiting for CS2 to start..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } while (!pid || pid == 0);

    HANDLE driver;
    do
    {
        system("cls");
        driver = CreateFile(L"\\\\.\\asumpte", GENERIC_READ, 0, nullptr, OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL, nullptr);
        if (driver == INVALID_HANDLE_VALUE) {
            std::cout << "Waiting for driver mapping..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } while (!driver || driver == INVALID_HANDLE_VALUE);

    do {
        system("cls");
        std::cout << "Waiting for driver to attach to process..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    } while (!driver::attach_to_process(driver, pid));

    std::uintptr_t client;
    do {
        system("cls");
        client = memory::get_module_base(pid, L"client.dll");
        if (!client || client == 0) {
            std::cout << "Waiting for game modules..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } while (!client || client == 0);

    std::cout << "Attached successfully." << std::endl;

    // create window
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = window_procedure;
    wc.hInstance = reinterpret_cast<HINSTANCE>(GetWindowLongA(memory::get_window_handle_from_process_id(pid), (-6)));
    wc.lpszClassName = L"^^";

    RegisterClassExW(&wc);

    HWND overlay = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED,
        wc.lpszClassName,
        L"^^",
        WS_POPUP,
        0,
        0,
        screenWidth,
        screenHeight,
        nullptr,
        nullptr,
        wc.hInstance,
        nullptr
    );
    if (overlay == NULL) {
        std::cout << "Overlay could not be created!" << std::endl;
        return EXIT_FAILURE;
    }

    SetLayeredWindowAttributes(overlay, RGB(0, 0, 0), BYTE(255), LWA_ALPHA);

    {
        RECT client_area{};
        GetClientRect(overlay, &client_area);

        RECT window_area{};
        GetWindowRect(overlay, &window_area);

        POINT diff{};
        ClientToScreen(overlay, &diff);

        MARGINS margins{
            window_area.left + (diff.x - window_area.left),
            window_area.top + (diff.y - window_area.top),
            client_area.right,
            client_area.bottom,
        };
        DwmExtendFrameIntoClientArea(overlay, &margins);
    }

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferDesc.RefreshRate.Numerator = 75U; // fps
    sd.BufferDesc.RefreshRate.Numerator = 1U;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.SampleDesc.Count = 1U;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 2U;
    sd.OutputWindow = overlay;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    D3D_FEATURE_LEVEL levels[2]{
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0,
    };

    ID3D11Device* device{ nullptr };
    ID3D11DeviceContext* device_context{ nullptr };
    IDXGISwapChain* swap_chain{ nullptr };
    ID3D11RenderTargetView* render_target_view{ nullptr };
    D3D_FEATURE_LEVEL level{};

    // create device
    D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0U,
        levels,
        2U,
        D3D11_SDK_VERSION,
        &sd,
        &swap_chain,
        &device,
        &level,
        &device_context
    );

    ID3D11Texture2D* back_buffer{ nullptr };
    swap_chain->GetBuffer(0U, IID_PPV_ARGS(&back_buffer));

    if (back_buffer) {
        device->CreateRenderTargetView(back_buffer, nullptr, &render_target_view);
        back_buffer->Release();
    }
    else
        return 1;

    ShowWindow(overlay, 1);//cmd_show
    UpdateWindow(overlay);

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;

    ImGui_ImplWin32_Init(overlay);
    ImGui_ImplDX11_Init(device, device_context);

    // imgui theme
    ImGuiStyle& style = ImGui::GetStyle();

    style.Alpha = 1.0f;
    style.WindowPadding = ImVec2(6.5f, 2.700000047683716f);
    style.WindowRounding = 6.0f;
    style.WindowBorderSize = 1.0f;
    style.WindowMinSize = ImVec2(20.0f, 32.0f);
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    style.WindowMenuButtonPosition = ImGuiDir_None;
    style.ChildRounding = 0.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupRounding = 10.10000038146973f;
    style.PopupBorderSize = 1.0f;
    style.FramePadding = ImVec2(20.0f, 3.5f);
    style.FrameRounding = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.ItemSpacing = ImVec2(4.400000095367432f, 4.0f);
    style.ItemInnerSpacing = ImVec2(4.599999904632568f, 3.599999904632568f);
    style.IndentSpacing = 4.400000095367432f;
    style.ColumnsMinSpacing = 5.400000095367432f;
    style.ScrollbarSize = 8.800000190734863f;
    style.ScrollbarRounding = 9.0f;
    style.GrabMinSize = 9.399999618530273f;
    style.GrabRounding = 0.0f;
    style.TabRounding = 0.0f;
    style.TabBorderSize = 0.0f;
    style.ColorButtonPosition = ImGuiDir_Right;
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

    style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.4980392158031464f, 0.4980392158031464f, 0.4980392158031464f, 1.0f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.05098039284348488f, 0.03529411926865578f, 0.03921568766236305f, 1.0f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.0784313753247261f, 0.0784313753247261f, 0.0784313753247261f, 1.0f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.1019607856869698f, 0.1019607856869698f, 0.1019607856869698f, 0.5f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.1607843190431595f, 0.1490196138620377f, 0.1921568661928177f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.2784313857555389f, 0.250980406999588f, 0.3372549116611481f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.2784313857555389f, 0.250980406999588f, 0.3372549116611481f, 1.0f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.2784313857555389f, 0.250980406999588f, 0.3372549116611481f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.2784313857555389f, 0.250980406999588f, 0.3372549116611481f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0f, 0.0f, 0.0f, 0.5099999904632568f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.1372549086809158f, 0.1372549086809158f, 0.1372549086809158f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.01960784383118153f, 0.01960784383118153f, 0.01960784383118153f, 0.5299999713897705f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3098039329051971f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.407843142747879f, 0.407843142747879f, 0.407843142747879f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.5098039507865906f, 0.5098039507865906f, 0.5098039507865906f, 1.0f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.5450980663299561f, 0.4666666686534882f, 0.7176470756530762f, 1.0f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.2784313857555389f, 0.250980406999588f, 0.3372549116611481f, 1.0f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.2784313857555389f, 0.250980406999588f, 0.3372549116611481f, 1.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.2784313857555389f, 0.250980406999588f, 0.3372549116611481f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.3450980484485626f, 0.294117659330368f, 0.4588235318660736f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.3137255012989044f, 0.2588235437870026f, 0.4274509847164154f, 1.0f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.3176470696926117f, 0.2784313857555389f, 0.407843142747879f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.4156862795352936f, 0.364705890417099f, 0.529411792755127f, 1.0f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.4039215743541718f, 0.3529411852359772f, 0.5098039507865906f, 1.0f);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.4274509847164154f, 0.4274509847164154f, 0.4980392158031464f, 0.5f);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.2784313857555389f, 0.250980406999588f, 0.3372549116611481f, 1.0f);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.2784313857555389f, 0.250980406999588f, 0.3372549116611481f, 1.0f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.2784313857555389f, 0.250980406999588f, 0.3372549116611481f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.2784313857555389f, 0.250980406999588f, 0.3372549116611481f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.2784313857555389f, 0.250980406999588f, 0.3372549116611481f, 1.0f);
    style.Colors[ImGuiCol_Tab] = ImVec4(0.2784313857555389f, 0.250980406999588f, 0.3372549116611481f, 1.0f);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.3254902064800262f, 0.2862745225429535f, 0.4156862795352936f, 1.0f);
    style.Colors[ImGuiCol_TabActive] = ImVec4(0.4000000059604645f, 0.3490196168422699f, 0.5058823823928833f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.2784313857555389f, 0.250980406999588f, 0.3372549116611481f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.2784313857555389f, 0.250980406999588f, 0.3372549116611481f, 1.0f);
    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.6078431606292725f, 0.6078431606292725f, 0.6078431606292725f, 1.0f);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0f, 0.4274509847164154f, 0.3490196168422699f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.8980392217636108f, 0.6980392336845398f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.0f, 0.6000000238418579f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.3499999940395355f);
    style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 1.0f, 0.0f, 0.8999999761581421f);
    style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.2784313857555389f, 0.250980406999588f, 0.3372549116611481f, 1.0f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.699999988079071f);
    style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.2000000029802322f);
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.3499999940395355f);

    ImFontConfig font;
    font.FontDataOwnedByAtlas = false;

    io.Fonts->AddFontFromMemoryTTF((void*)rawData, sizeof(rawData), 17.0f, &font);

    do
    {

        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        MSG msg;
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT)
            {
                settings::running = false;
            }
        }

        const bool end_pressed = GetAsyncKeyState(VK_DELETE);
        if (end_pressed)
        {
            settings::running = false;
        }

        if (!settings::running)
            continue;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        const bool home_pressed = GetAsyncKeyState(VK_INSERT);
        if (home_pressed)
        {
            settings::showImGui = !settings::showImGui;
            if (settings::showImGui)
            {
                LONG_PTR exStyle = GetWindowLongPtr(overlay, GWL_EXSTYLE);
                exStyle &= ~WS_EX_TRANSPARENT; // remove WS_EX_TRANSPARENT
                SetWindowLongPtr(overlay, GWL_EXSTYLE, exStyle);
            }
            else
            {
                LONG_PTR exStyle = GetWindowLongPtr(overlay, GWL_EXSTYLE);
                exStyle |= WS_EX_TRANSPARENT; // add WS_EX_TRANSPARENT
                SetWindowLongPtr(overlay, GWL_EXSTYLE, exStyle);
            }
            Sleep(400);
        }

        // RGB enemy
        static int esp_enemy_red = 255;
        static int esp_enemy_green = 0;
        static int esp_enemy_blue = 255;

        // RGB team
        static int esp_team_red = 0;
        static int esp_team_green = 160;
        static int esp_team_blue = 255;

        // AIM FOV Circle
        static int aimbotFovCircleRed = 255;
        static int aimbotFovCircleGreen = 0;
        static int aimbotFovCircleBlue = 0;

        if (settings::showImGui)
        {
            ImGui::SetNextWindowSize(ImVec2(330, 300));
            ImGui::Begin("asumpte", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

            ImGui::BeginTabBar("Options");

            static ImVec4 aim_fov_circle = ImVec4(esp_team_red / 255.0f, esp_team_green / 255.0f, esp_team_blue / 255.0f, 1.0f);
            if (ImGui::BeginTabItem("Aimbot")) {
                ImGui::Checkbox("Enable Aimbot", &settings::aimbot);
                ImGui::Checkbox("Memory Aimbot", &settings::aimbotMemory);
                ImGui::SliderInt("Aimbot FOV", &settings::aimbotFovSize, 1, 180);
                ImGui::SliderFloat("Aimbot Head Slider", &settings::aimbotheadSlider, 1, 100);
                ImGui::Checkbox("Draw Circle FOV", &settings::aimbotFov);
                ImGui::ColorEdit3("Circle Color", (float*)&aim_fov_circle);

                aimbotFovCircleRed = static_cast<int>(aim_fov_circle.x * 255.0f);
                aimbotFovCircleGreen = static_cast<int>(aim_fov_circle.y * 255.0f);
                aimbotFovCircleBlue = static_cast<int>(aim_fov_circle.z * 255.0f);

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("ESP")) {
                ImGui::Checkbox("Enable ESP", &settings::enableEsp);
                ImGui::Checkbox("Draw Lines", &settings::drawLines);

                static ImVec4 esp_color = ImVec4(esp_enemy_red / 255.0f, esp_enemy_green / 255.0f, esp_enemy_blue / 255.0f, 1.0f);
                ImGui::ColorEdit3("Enemy Color", (float*)&esp_color);

                esp_enemy_red = static_cast<int>(esp_color.x * 255.0f);
                esp_enemy_green = static_cast<int>(esp_color.y * 255.0f);
                esp_enemy_blue = static_cast<int>(esp_color.z * 255.0f);

                static ImVec4 esp_team_color = ImVec4(esp_team_red / 255.0f, esp_team_green / 255.0f, esp_team_blue / 255.0f, 1.0f);
                ImGui::ColorEdit3("Team Color", (float*)&esp_team_color);

                esp_team_red = static_cast<int>(esp_team_color.x * 255.0f);
                esp_team_green = static_cast<int>(esp_team_color.y * 255.0f);
                esp_team_blue = static_cast<int>(esp_team_color.z * 255.0f);

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Misc")) {

                ImGui::Checkbox("Treat team as enemy", &settings::showTeam);

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();

            ImGui::End();
        }

        RGB esp_enemy_color = { esp_enemy_red, esp_enemy_green, esp_enemy_blue };
        RGB esp_team_color = { esp_team_red, esp_team_green, esp_team_blue };
        RGB aim_fov_circle_color = { aimbotFovCircleRed, aimbotFovCircleGreen, aimbotFovCircleBlue };

        const auto local_player_pawn = driver::read_memory<uintptr_t>(driver, client + cs2_dumper::offsets::client_dll::dwLocalPlayerPawn);
        const auto local_origin = driver::read_memory<Vector3>(driver, local_player_pawn + cs2_dumper::schemas::client_dll::C_BasePlayerPawn::m_vOldOrigin);
        const auto view_matrix = driver::read_memory<view_matrix_t>(driver, client + cs2_dumper::offsets::client_dll::dwViewMatrix);
        const auto entity_list = driver::read_memory<uintptr_t>(driver, client + cs2_dumper::offsets::client_dll::dwEntityList);
        const auto local_team = driver::read_memory<int>(driver, local_player_pawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iTeamNum);
        const auto flags = driver::read_memory<std::uint32_t>(driver, local_player_pawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_fFlags);

        if (settings::aimbotFov) {
            Render::DrawCircle(screenWidth / 2, screenHeight / 2, settings::aimbotFovSize / 180.f * screenWidth, aim_fov_circle_color, 1);
        }

		int centerWidth = screenWidth / 2;
		int centerHeight = screenHeight / 2;
        for (int playerIndex = 1; playerIndex < 64; ++playerIndex) {
            uintptr_t listEntry = driver::read_memory<uintptr_t>(driver, entity_list + (8 * (playerIndex & 0x7FFF) >> 9) + 16);
            if (!listEntry)
                continue;

            uintptr_t player = driver::read_memory<uintptr_t>(driver, listEntry + 120 * (playerIndex & 0x1FF));
            if (!player)
                continue;

            int playerTeam = driver::read_memory<int>(driver, player + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iTeamNum);
            if (!settings::showTeam) {
                if (playerTeam == local_team)
                    continue;
            }

            uint32_t playerPawn = driver::read_memory<uint32_t>(driver, player + cs2_dumper::schemas::client_dll::CCSPlayerController::m_hPlayerPawn);
            uintptr_t pawnListEntry = driver::read_memory<uintptr_t>(driver, entity_list + 0x8 * ((playerPawn & 0x7FFF) >> 9) + 16);
            if (!pawnListEntry)
                continue;

            uintptr_t pCSPlayerPawn = driver::read_memory<uintptr_t>(driver, pawnListEntry + 120 * (playerPawn & 0x1FF));
            if (!pCSPlayerPawn)
                continue;

            int health = driver::read_memory<int>(driver, pCSPlayerPawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iHealth);
            if (health <= 0)
                continue;

            if (pCSPlayerPawn == local_player_pawn)
                continue;

            Vector3 origin = driver::read_memory<Vector3>(driver, pCSPlayerPawn + cs2_dumper::schemas::client_dll::C_BasePlayerPawn::m_vOldOrigin);
            Vector3 head = { origin.x, origin.y, origin.z + 75.f };

            Vector3 screenPos = origin.WTS(view_matrix);
            Vector3 screenHead = head.WTS(view_matrix);

            float height = screenPos.y - screenHead.y;
            float width = height / 2.4f;

            int rectBottomX = screenHead.x;
            int rectBottomY = screenHead.y + height;

            if (settings::aimbot && GetAsyncKeyState(VK_RBUTTON)) {

                Vector3 playerView = local_origin + driver::read_memory<Vector3>(driver, local_player_pawn + cs2_dumper::schemas::client_dll::C_BaseModelEntity::m_vecViewOffset);
                Vector3 entityView = origin + driver::read_memory<Vector3>(driver, pCSPlayerPawn + cs2_dumper::schemas::client_dll::C_BaseModelEntity::m_vecViewOffset);

                Vector3 newHead = { head.x, head.y, origin.z + settings::aimbotheadSlider };

                Vector2 newAngles = Vector3::CalculateAngles(playerView, newHead);
                Vector3 newAngelsVec3 = { newAngles.y, newAngles.x, 0.f };

				Vector3 angelsScreen = newAngelsVec3.WTS(view_matrix);

                float distance = sqrt(pow(screenHead.x - screenWidth / 2, 2) + pow(screenHead.y - screenHeight / 2, 2));
                if (distance <= settings::aimbotFovSize / 180.f * screenWidth) {

                    //if (settings::aimbotMemory) {
                    //    driver::write_memory<Vector3>(driver, client + cs2_dumper::offsets::client_dll::dwViewAngles, newAngelsVec3);
                    //}
                    //else 
                    //{

                        //FIX THIS, better use kerneldriver for that cuz cs aint allowing to do so

                        int x = screenHead.x + settings::aimbotheadSlider;
                        int y = screenHead.y + height;
						std::cout << "Mouse Event: " << x << ", " << y << std::endl;

                        INPUT input;
                        input.type = INPUT_MOUSE;
                        input.mi.dx = x * 65536 / GetSystemMetrics(SM_CXSCREEN);
                        input.mi.dy = y * 65536 / GetSystemMetrics(SM_CYSCREEN);
                        input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
                        input.mi.mouseData = 0;
                        input.mi.dwExtraInfo = NULL;
                        input.mi.time = 0;

                        SendInput(1, &input, sizeof(INPUT));
                    //}
                }  
            }

            if (settings::enableEsp && screenHead.x - width / 2 >= 0 &&
                screenHead.x + width / 2 <= screenWidth &&
                screenHead.y >= 0 &&
                screenHead.y + height <= screenHeight &&
                screenHead.z > 0) {

                int bottomCenterX = screenWidth / 2;
                int bottomCenterY = screenHeight;

                if (playerTeam == local_team) {
                    // teammates with team color
                    if (settings::drawLines) {
                        Render::DrawLine(bottomCenterX, bottomCenterY, rectBottomX, rectBottomY, esp_team_color, 1.5f);
                    }
                    Render::DrawRect(
                        screenHead.x - width / 2,
                        screenHead.y,
                        width,
                        height,
                        esp_team_color,
                        1.5
                    );
                }
                else {
                    // enemies with enemy color
                    if (settings::drawLines) {
                        Render::DrawLine(bottomCenterX, bottomCenterY, rectBottomX, rectBottomY, esp_enemy_color, 1.5f);
                    }
                    Render::DrawRect(
                        screenHead.x - width / 2,
                        screenHead.y,
                        width,
                        height,
                        esp_enemy_color,
                        1.5
                    );
                }
            }
        }

        ImGui::Render();
        float color[4]{ 0,0,0,0 };
        device_context->OMSetRenderTargets(1U, &render_target_view, nullptr);
        device_context->ClearRenderTargetView(render_target_view, color);

        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        swap_chain->Present(0U, 0U);

    } while (settings::running);

    std::cout << "Exiting..." << std::endl;

    // exiting
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplDX11_Shutdown();

    ImGui::DestroyContext();

    if (swap_chain) {
        swap_chain->Release();
    }

    if (device_context) {
        device_context->Release();
    }

    if (device) {
        device->Release();
    }

    if (render_target_view)
    {
        render_target_view->Release();
    }

    DestroyWindow(overlay);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);

    // close handle to driver
    CloseHandle(driver);

    return EXIT_SUCCESS; // successful execution
}
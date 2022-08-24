#pragma once
#include <tchar.h>

#include <imgui.h>
#include <imgui_internal.h>
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include <d3d9.h>

#include "imgui_backend.h"
#include "../main.h"
#include "editors.h"
#include "../winAPI.h"
#include "../memory-editing.h"

// DirectX 9 functions
LPDIRECT3D9              g_pD3D = NULL;
LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
D3DPRESENT_PARAMETERS    g_d3dpp = {};

bool CreateDeviceD3D(HWND hWnd)
{
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
        return false;

    // Create the D3DDevice
    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed = TRUE;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN; // Need to use an explicit format with alpha if needing per-pixel alpha composition.
    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;           // Present with vsync
    //g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate
    if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
        return false;

    return true;
}

void CleanupDeviceD3D()
{
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
    if (g_pD3D) { g_pD3D->Release(); g_pD3D = NULL; }
}

void ResetDevice()
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    HRESULT hresult = g_pd3dDevice->Reset(&g_d3dpp);
    if (hresult == D3DERR_INVALIDCALL)
        IM_ASSERT(0);
    ImGui_ImplDX9_CreateDeviceObjects();
}

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0 // From Windows SDK 8.1+ headers
#endif

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            g_d3dpp.BackBufferWidth = LOWORD(lParam);
            g_d3dpp.BackBufferHeight = HIWORD(lParam);
            ResetDevice();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    case WM_DPICHANGED:
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports)
        {
            //const int dpi = HIWORD(wParam);
            //printf("WM_DPICHANGED to %d (%.0f%%)\n", dpi, (float)dpi / 96.0f * 100.0f);
            const RECT* suggested_rect = (RECT*)lParam;
            ::SetWindowPos(hWnd, NULL, suggested_rect->left, suggested_rect->top, suggested_rect->right - suggested_rect->left, suggested_rect->bottom - suggested_rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
        }
        break;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
// End DirectX 9 functions


using std::cout;

short timer;
bool GamePaused = false;
bool OpenedAttackSkill = false;
short ID = 0;

int CreateUI() {
    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("Skill Editor"), NULL };
    ::RegisterClassEx(&wc);
    HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("Phantom Dust Skill Editor"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);

    // Loads font data from font.h
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 19.0f);

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Start the Dear ImGui frame
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGuiViewportP* viewport = (ImGuiViewportP*)(void*)ImGui::GetMainViewport();
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
        float height = ImGui::GetFrameHeight();

        if (ImGui::BeginViewportSideBar("MenuBar", viewport, ImGuiDir_Up, height, window_flags)) {
            if (ImGui::BeginMenuBar()) {
                if (ImGui::BeginMenu("File"))
                {
                    if (ImGui::MenuItem("New Skill Pack", "Ctrl + N"))
                    {
                        SafeNewPack();
                    }
                    if (ImGui::BeginMenu("Open"))
                    {
                        if (ImGui::MenuItem("Skill (From Memory)"))
                        {
                            if (GetProcess())
                            {
                                UI.IDSelection = true;
                            }
                        }
                        if (ImGui::MenuItem("Skill File"))
                        {
                            if (FileSelectDialog(skillfile) != -1) // Open a file open dialog
                            {
                                LoadAttackSkill();        // Loads the current file into the atkskill struct
                                cout << "Imported attack skill " << filepath << "\n";
                                OpenedAttackSkill = true;
                                UI.AttackSkillEditor = true;    // Opens the Attack Skill Editor window
                            }
                            else
                            {
                                cout << "File selection canceled.\n";
                                UI.ErrorCode = 1;
                            }
                        }
                        if (ImGui::MenuItem("Install Skill Pack"))
                        {
                            GetProcess();
                            if (SUCCEEDED(MultiSelectWindow())) // Open a multiple file open dialog
                            {
                                InstallSkillPackToRAM();
                                for (int i = 0; i < MultiSelectCount; i++)
                                {
                                    cout << "Installed skill pack " << multiselectpath[i] << ".\n";
                                }
                            }
                            else
                            {
                                cout << "File selection canceled.\n";
                                UI.ErrorCode = 1;
                            }
                        }
                        if (DebugMode)
                        {
                            if (ImGui::MenuItem("Install Skill Pack to Disk"))
                            {
                                if (SUCCEEDED(MultiSelectWindow())) // Open a multiple file open dialog
                                {
                                    // InstallSkillPack();
                                    for (int i = 0; i < MultiSelectCount; i++)
                                    {
                                        cout << "Installed skill pack " << multiselectpath[i] << ".\n";
                                    }
                                }
                                else
                                {
                                    cout << "File selection canceled.\n";
                                    UI.ErrorCode = 1;
                                }
                            }
                        }
                        ImGui::EndMenu();
                    }
                    if (ImGui::MenuItem("Save", "Ctrl + S"))
                    {
                        SaveAtkSkill();
                    }
                    if (ImGui::MenuItem("Save As", "Ctrl + Shift + S"))
                    {
                        if (FileSaveDialog(skillfile, L".skill") != -1) // Open a file save dialog and save to a new file
                        {
                            SaveAtkSkill(); // Write data.
                        }
                        else
                        {
                            cout << "File selection canceled.\n";
                        }
                    }

                    if (ImGui::MenuItem("Exit", "Alt + F4"))
                    {
                        return 0;
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Window"))
                {
                    if (ImGui::MenuItem("Attack Skill Editor"))
                    {
                        UI.AttackSkillEditor = !UI.AttackSkillEditor; // Toggle Attack Skill Editor window
                    }
                    if (ImGui::MenuItem("Skill Hex Editor"))
                    {
                        GetProcess();
                        if (LoadGSDataFromRAM() == 0)
                        {
                            UI.HexEditor = !UI.HexEditor;
                        }
                    }
                    if (ImGui::MenuItem("Documentation"))
                    {
                        UI.Documentation = !UI.Documentation;
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Game"))
                {
                    if (ImGui::MenuItem("Freeze/Unfreeze Phantom Dust"))
                    {
                        if (GamePaused) {
                            UnpauseGame();
                            GamePaused = true;
                        }
                        else {
                            PauseGame();
                        }
                        GamePaused = !GamePaused;
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }
            ImGui::End();
        }

        if (timer == 0)
        {
            // Save: Ctrl + S
            if (GetKeyState(VK_CONTROL) & GetKeyState('S') & 0x8000)
            {
                timer = 20;

                // Save As: Ctrl + Shift + S
                if (GetKeyState(VK_SHIFT))
                {
                    if (FileSaveDialog(skillfile, L".skill") != -1)
                    {
                        SaveAtkSkill(); // Write data.
                    }
                    else
                    {
                        cout << "File selection cancelled.\n";
                    }
                }
                else
                {
                    SaveAtkSkill(); // Write data.
                }
            }

            // New: Ctrl + N
            if (GetKeyState(VK_CONTROL) & GetKeyState('N') & 0x8000 && !UI.NewSkillPack)
            {
                SafeNewPack();
                timer = 20;
            }
        }
        else
        {
            timer--; // Decrement cooldown timer until it hits 0
        }

        if (UI.IDSelection)
        {
            ImGui::Begin("Input a skill ID: ");
            InputShort("ID", &ID);

            if (ImGui::Button("Open"))
            {
                memcpy(&AtkSkill, &skillarray[ID], 144);
                cout << "Loaded skill with ID " << ID << ".\n";
                OpenedAttackSkill = true;
                UI.IDSelection = false;      // Close this window
                UI.AttackSkillEditor = true; // Opens the Attack Skill Editor window
            }

            ImGui::End();
        }

        if (UI.HexEditor)
        {
            HexEditorWindow(ID);
        }

        if (UI.AttackSkillEditor)
        {
            AtkSkillWindow();
        }

        if (UI.Documentation)
        {
            DocumentationWindow();
        }


        if (UI.NewSkillPack)
        {
            SkillPackWindow();
        }

        // Rendering
        ImGui::EndFrame();
        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(clear_color.x * clear_color.w * 255.0f), (int)(clear_color.y * clear_color.w * 255.0f), (int)(clear_color.z * clear_color.w * 255.0f), (int)(clear_color.w * 255.0f));
        g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
        if (g_pd3dDevice->BeginScene() >= 0)
        {
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            g_pd3dDevice->EndScene();
        }

        // Update and Render additional Platform Windows
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        HRESULT result = g_pd3dDevice->Present(NULL, NULL, NULL, NULL);

        // Handle loss of D3D9 device
        if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
            ResetDevice();
    }

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}
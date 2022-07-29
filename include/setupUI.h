#pragma once
#include <iostream>
#include <tchar.h>

#include "imgui.h"
#include <imgui_internal.h>
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include <d3d9.h>

#include <dx-init.h>
#include <main.h>

bool OpenedAtkSkill = false;

AttackSkill AtkSkill;

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

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("include/imGUI/misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("include/imGUI/misc/fonts/Cousine-Regular.ttf", 15.0f);
    io.Fonts->AddFontFromFileTTF("include/imGUI/misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("include/imGUI/misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    // Our state
    bool show_demo_window = false;
    bool show_another_window = false;
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

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
        {
            ImGuiViewportP* viewport = (ImGuiViewportP*)(void*)ImGui::GetMainViewport();
            ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
            float height = ImGui::GetFrameHeight();

            if (ImGui::BeginViewportSideBar("MenuBar", viewport, ImGuiDir_Up, height, window_flags)) {
                if (ImGui::BeginMenuBar()) {
                    if (ImGui::BeginMenu("File"))
                    {
                        if (ImGui::MenuItem("Open"))
                        {
                            FileSelectDialog(NULL, NULL, NULL, NULL);
                            LoadAttackSkill(filepathptr);
                            cout << "Imported attack skill " << filepath << ".";
                            OpenedAtkSkill = true;
                        }
                        if (ImGui::MenuItem("Save"))
                        {
                            FileSaveDialog(NULL, NULL, NULL, NULL);
                            ofstream AtkSkillFile(filepathptr, ios::binary);
                            AtkSkillFile.write((char*)&AtkSkill, 144);
                        }

                        if (ImGui::MenuItem("Exit"))
                        {
                            return 1;
                        }
                        ImGui::EndMenu();
                    }
                    if (ImGui::BeginMenu("Edit"))
                    {
                        if (ImGui::MenuItem("Attack Skill Editor"))
                        {
                            OpenedAtkSkill = true;
                        }
                        ImGui::EndMenu();
                    }
                    ImGui::EndMenuBar();
                }
                ImGui::End();
            }

            if (ImGui::BeginViewportSideBar("StatusBar", viewport, ImGuiDir_Down, height, window_flags)) {
                if (ImGui::BeginMenuBar()) {
                    if (OpenedAtkSkill) {
                        string print = "Imported attack skill " + filepath;
                        ImGui::Text(const_cast<char*>(print.c_str()));
                    }
                    ImGui::EndMenuBar();
                }
                ImGui::End();
            }

            if (OpenedAtkSkill)
            {
                const short s16_one = 1;
                const uint8_t s8_one = 1;
                ImGui::Begin("Attack Skill Editor");
                ImGui::InputScalar("Skill Text ID", ImGuiDataType_S16, &AtkSkill.SkillTextID, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("Register ID", ImGuiDataType_S16, &AtkSkill.RegisterID, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("Skill ID", ImGuiDataType_S16, &AtkSkill.SkillID, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("Rarity Stars", ImGuiDataType_S16, &AtkSkill.RarityStars, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("Sound File ID", ImGuiDataType_S16, &AtkSkill.SoundFileID, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("Capsule Type", ImGuiDataType_S16, &AtkSkill.CapsuleType, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("School ID", ImGuiDataType_S16, &AtkSkill.SchoolID, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("Animation ID (Ground)", ImGuiDataType_S16, &AtkSkill.AnimationIDGround, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("Animation ID (Air)", ImGuiDataType_S16, &AtkSkill.AnimationIDAir, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("Multi Press 1", ImGuiDataType_S16, &AtkSkill.MultiPress1, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("Multi Press 2", ImGuiDataType_S16, &AtkSkill.MultiPress2, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("Double Skill 1", ImGuiDataType_S16, &AtkSkill.DoubleSkill1, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("Double Skill 2", ImGuiDataType_S16, &AtkSkill.DoubleSkill2, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("After Hit SFX", ImGuiDataType_S16, &AtkSkill.PostHitSFX, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("Start Up SFX", ImGuiDataType_S16, &AtkSkill.StartUpSFX, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("Collision SFX", ImGuiDataType_S16, &AtkSkill.CollisionSFX, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("Aura Cost", ImGuiDataType_S16, &AtkSkill.Cost, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("Additional Aura Cost", ImGuiDataType_S16, &AtkSkill.ExtraCost, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("Health Penalty", ImGuiDataType_S16, &AtkSkill.HealthCost, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("# of Uses", ImGuiDataType_S16, &AtkSkill.SkillUses, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("Self Effect", ImGuiDataType_S16, &AtkSkill.SelfEffect, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("Button Restrictions", ImGuiDataType_S16, &AtkSkill.ButtonRestrictions, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("Requirements", ImGuiDataType_S16, &AtkSkill.Requirements, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("Requirement Amount", ImGuiDataType_S16, &AtkSkill.ReqAmount, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("Ground/Air/Both", ImGuiDataType_S16, &AtkSkill.GroundAirBoth, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("Skill Button Effect", ImGuiDataType_S16, &AtkSkill.SkillButtonEffect, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("Applied Status ID", ImGuiDataType_S16, &AtkSkill.AppliedStatusID, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("Restrictions", ImGuiDataType_S16, &AtkSkill.Restriction, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("Strength Effect", ImGuiDataType_S16, &AtkSkill.StrengthEffect, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("Damage", ImGuiDataType_S16, &AtkSkill.Damage, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("Effect Duration / Misc. Effects", ImGuiDataType_S16, &AtkSkill.EffectDuration, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("Target Hand Data", ImGuiDataType_S16, &AtkSkill.TargetHand, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("Hit Effect Skills", ImGuiDataType_S16, &AtkSkill.HitEffectSkills, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("Increase Stat", ImGuiDataType_S16, &AtkSkill.Increase, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("Status Enabler", ImGuiDataType_S8, &AtkSkill.StatusEnabler, true ? &s8_one : NULL, NULL, "%d");
                ImGui::InputScalar("Status ID Duration", ImGuiDataType_S8, &AtkSkill.StatusDuration, true ? &s8_one : NULL, NULL, "%d");
                ImGui::InputScalar("Projectile Properties", ImGuiDataType_S16, &AtkSkill.ProjectileProperties, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("Projectile ID", ImGuiDataType_S16, &AtkSkill.ProjectileID, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("\"Collision Skill ID\"", ImGuiDataType_S16, &AtkSkill.ProjectileID, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("Homing Range 1st Hit", ImGuiDataType_S16, &AtkSkill.HomingRangeFirstHit, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("Knockback Strength", ImGuiDataType_S16, &AtkSkill.HomingRangeSecondHit, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("Combo End", ImGuiDataType_S16, &AtkSkill.HomingRangeThirdHit, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("Skill Duration", ImGuiDataType_S8, &AtkSkill.SkillDuration, true ? &s8_one : NULL, NULL, "%d");
                ImGui::InputScalar("Hit Range", ImGuiDataType_S8, &AtkSkill.HitRange, true ? &s8_one : NULL, NULL, "%d");
                ImGui::InputScalar("Expand Skill Width / Start Speed", ImGuiDataType_S16, &AtkSkill.ExpandSkillWidth, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("Animation Size / Acceleration", ImGuiDataType_S16, &AtkSkill.AnimationSize, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("Projectile End Speed", ImGuiDataType_S16, &AtkSkill.ProjectileSpeed, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("Homing Strength / Accuracy", ImGuiDataType_S16, &AtkSkill.AccuracyID, true ? &s16_one : NULL, NULL, "%d");
                ImGui::InputScalar("Animation Height", ImGuiDataType_S16, &AtkSkill.AnimationHeight, true ? &s16_one : NULL, NULL, "%d");
                if (ImGui::Button("Close")) {
                    OpenedAtkSkill = false;
                }
                ImGui::End();
            }
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
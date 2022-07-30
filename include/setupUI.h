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
#include <font.h>

string print;
short ErrorCode;
short AtkSkillState = 0; // 0 = None, 1 = Opened, 2 = Saved
bool AtkSkillWindow = false;

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
    ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(DroidSans_compressed_data_base85, 16.0f);
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
                            if (FileSelectDialog(NULL, NULL, NULL, NULL) != -1) // Pulls up file open window and places the selected
                            {                                                   // file's path in the filepathptr and filepath variables
                                LoadAttackSkill(filepathptr);                   // Loads the selected file into memory and loads
                                                                                // the raw data into the variables of the atkskill struct
                                cout << "Imported attack skill " << filepath << "\n";
                                AtkSkillState = 1;                              // Causes a message to appear on the status bar
                                AtkSkillWindow = true;                          // Opens the Attack Skill Editor window
                            }
                            else
                            {
                                cout << "File selection cancelled.\n";
                                ErrorCode = 1;
                            }
                        }
                        if (ImGui::MenuItem("Save"))
                        {
                            if (AtkSkillState != 0) {
                                ofstream AtkSkillFile(filepathptr, ios::binary);  // Create a new ofstream variable, using
                                                                                  // the name of the file that was opened.
                                AtkSkillFile.write((char*)&AtkSkill, 144);        // Overwrites the file that was opened with
                                                                                  // the new data.
                                cout << "Saved attack skill to " << filepath << "\n";
                                AtkSkillState = 2;                                // Causes a message to appear on the status bar
                            }
                            else {
                                cout << "Tried to save without opening a file, aborting...\n";
                                ErrorCode = 2;
                            }
                        }
                        if (ImGui::MenuItem("Save As"))
                        {
                            if (AtkSkillState != 0) {
                                if (FileSaveDialog(NULL, NULL, NULL, NULL) != -1)    // Open a file save dialog and save to this
                                {                                                    // new file instead of overwriting the original.
                                    ofstream AtkSkillFile(filepathptr, ios::binary);
                                    AtkSkillFile.write((char*)&AtkSkill, 144);       // Write data.
                                    cout << "Saved attack skill to " << filepath << "\n";
                                    AtkSkillState = 2;
                                }
                                else
                                {
                                    cout << "File selection cancelled.\n";
                                    ErrorCode = 1;
                                }
                            }
                            else {
                                cout << "Tried to save without opening a file, aborting...\n";
                                ErrorCode = 2;
                            }
                        }

                        if (ImGui::MenuItem("Exit", "Alt + F4"))
                        {
                            return 0;
                        }
                        ImGui::EndMenu();
                    }
                    if (ImGui::BeginMenu("Edit"))
                    {
                        if (ImGui::MenuItem("Attack Skill Editor"))
                        {
                            AtkSkillWindow = true; // Opens the Attack Skill Editor window
                        }
                        ImGui::EndMenu();
                    }
                    ImGui::EndMenuBar();
                }
                ImGui::End();
            }

            if (ImGui::BeginViewportSideBar("StatusBar", viewport, ImGuiDir_Down, height, window_flags)) {
                if (ImGui::BeginMenuBar()) {
                    if (AtkSkillState == 1) {
                        print = "Imported attack skill " + filepath; // Status messages about importing
                                                                     // and saving files.
                    }
                    else if (AtkSkillState == 2) {
                        print = "Saved attack skill to " + filepath;
                    }
                    else if (ErrorCode == 1) {
                        print = "File selection cancelled.";
                    }
                    else if (ErrorCode == 2) {
                        print = "Tried to save without opening a file, aborting...";
                    }
                    ImGui::Text(const_cast<char*>(print.c_str()));
                    ImGui::EndMenuBar();
                }
                ImGui::End();
            }

            if (AtkSkillWindow)
            {
                const short s16_one = 1;
                const uint8_t s8_one = 1;
                string WindowTitle = "Attack Skill Editor - " + filepath; // Use filename in the window title.
                ImGui::Begin(const_cast<char*>(WindowTitle.c_str()));     // TODO: chop off the filepath and use only the name.

                InputShort("Skill Text ID", &AtkSkill.SkillTextID);       // See comments in the atkskill struct for
                InputShort("Register ID", &AtkSkill.RegisterID);          // more info on what these mean.There are
                InputShort("Skill ID", &AtkSkill.SkillID);                // also lots of unknown data sections not
                InputShort("Rarity Stars", &AtkSkill.RarityStars);        // listed here yet.
                InputShort("Sound File ID", &AtkSkill.SoundFileID);
                InputShort("Capsule Type", &AtkSkill.CapsuleType);
                InputShort("School ID", &AtkSkill.SchoolID);
                InputShort("Animation ID (Ground)", &AtkSkill.AnimationIDGround);
                InputShort("Animation ID (Air)", &AtkSkill.AnimationIDAir);
                InputShort("Multi Press 1", &AtkSkill.MultiPress1);
                InputShort("Multi Press 2", &AtkSkill.MultiPress2);
                InputShort("Double Skill 1", &AtkSkill.DoubleSkill1);
                InputShort("Double Skill 2", &AtkSkill.DoubleSkill2);
                InputShort("After Hit SFX", &AtkSkill.PostHitSFX);
                InputShort("Start Up SFX", &AtkSkill.StartUpSFX);
                InputShort("Collision SFX", &AtkSkill.CollisionSFX);
                InputShort("Aura Cost", &AtkSkill.Cost);
                InputShort("Additional Aura Cost", &AtkSkill.ExtraCost);
                InputShort("Health Penalty", &AtkSkill.HealthCost);
                InputShort("# of Uses", &AtkSkill.SkillUses);
                InputShort("Self Effect", &AtkSkill.SelfEffect);
                InputShort("Button Restrictions", &AtkSkill.ButtonRestrictions);
                InputShort("Requirements", &AtkSkill.Requirements);
                InputShort("Requirement Amount", &AtkSkill.ReqAmount);
                InputShort("Ground/Air/Both", &AtkSkill.GroundAirBoth);
                InputShort("Skill Button Effect", &AtkSkill.SkillButtonEffect);
                InputShort("Applied Status ID", &AtkSkill.AppliedStatusID);
                InputShort("Restrictions", &AtkSkill.Restriction);
                InputShort("Strength Effect", &AtkSkill.StrengthEffect);
                InputShort("Damage", &AtkSkill.Damage);
                InputShort("Effect Duration / Misc. Effects", &AtkSkill.EffectDuration);
                InputShort("Target Hand Data", &AtkSkill.TargetHand);
                InputShort("Hit Effect Skills", &AtkSkill.HitEffectSkills);
                InputShort("Increase Stat", &AtkSkill.Increase);
                InputUInt8("Status Enabler", &AtkSkill.StatusEnabler);
                InputUInt8("Status ID Duration", &AtkSkill.StatusDuration);
                InputShort("Projectile Properties", &AtkSkill.ProjectileProperties);
                InputShort("Projectile ID", &AtkSkill.ProjectileID);
                InputShort("\"Collision Skill ID\"", &AtkSkill.ProjectileID);
                InputShort("Homing Range 1st Hit", &AtkSkill.HomingRangeFirstHit);
                InputShort("Knockback Strength", &AtkSkill.HomingRangeSecondHit);
                InputShort("Combo End", &AtkSkill.HomingRangeThirdHit);
                InputUInt8("Skill Duration", &AtkSkill.SkillDuration);
                InputUInt8("Hit Range", &AtkSkill.HitRange);
                InputShort("Expand Skill Width / Start Speed", &AtkSkill.ExpandSkillWidth);
                InputShort("Animation Size / Acceleration", &AtkSkill.AnimationSize);
                InputShort("Projectile End Speed", &AtkSkill.ProjectileSpeed);
                InputShort("Homing Strength / Accuracy", &AtkSkill.AccuracyID);
                InputShort("Animation Height", &AtkSkill.AnimationHeight);
                if (ImGui::Button("Close")) {
                    AtkSkillWindow = false; // Deactivates the window.
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
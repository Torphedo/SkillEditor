#pragma once
#include <main.h>
#include <UI.h>

short ErrorCode;
bool OptionsWindow = false;
bool SkillPackWindow = false;
short AtkSkillState = 0; // 0 = None, 1 = Opened, 2 = Saved
bool AtkSkillWindow = false;

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
    ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(DroidSans_compressed_data_base85, 16.0f);

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
                    if (ImGui::MenuItem("New Skill Pack"))
                    {
                        if (SUCCEEDED(MultiSelectWindow()))
                        {
                            SkillPackWindow = true;
                        }
                        else {
                            cout << "Skill selection canceled.\n";
                            ErrorCode = 2;
                        }
                    }
                    if (ImGui::BeginMenu("Open"))
                    {
                        if (ImGui::MenuItem("Skill"))
                        {
                            if (FileSelectDialog() != -1) // Open a file open dialog
                            {
                                LoadAttackSkill();        // Loads the current file into the atkskill struct
                                cout << "Imported attack skill " << filepath << "\n";
                                AtkSkillState = 1;        // Causes a message to appear on the status bar
                                AtkSkillWindow = true;    // Opens the Attack Skill Editor window
                            }
                            else
                            {
                                cout << "File selection canceled.\n";
                                ErrorCode = 1;
                            }
                        }
                        if (ImGui::MenuItem("Install Skill Pack"))
                        {
                            if (SUCCEEDED(MultiSelectWindow())) // Open a multiple file open dialog
                            {
                                InstallSkillPack();
                                for (unsigned int i = 0; i < MultiSelectCount; i++)
                                {
                                    cout << "Installed skill pack " << multiselectpath[i] << ".\n";
                                }
                            }
                            else
                            {
                                cout << "File selection canceled.\n";
                                ErrorCode = 1;
                            }
                        }
                        ImGui::EndMenu();
                    }
                    if (ImGui::MenuItem("Save"))
                    {
                        if (AtkSkillState != 0) {
                            SaveAtkSkill();    // Write data.
                            cout << "Saved attack skill to " << filepath << "\n";
                            AtkSkillState = 2; // Causes a message to appear on the status bar
                        }
                        else {
                            cout << "Tried to save without opening a file, aborting...\n";
                            ErrorCode = 2;
                        }
                    }
                    if (ImGui::MenuItem("Save As"))
                    {
                        if (AtkSkillState != 0) {
                            if (FileSaveDialog() != -1) // Open a file save dialog and save to a new file
                            {
                                SaveAtkSkill();         // Write data.
                                cout << "Saved attack skill to " << filepath << "\n";
                                AtkSkillState = 2;
                            }
                            else
                            {
                                cout << "File selection canceled.\n";
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
                    if (ImGui::MenuItem("Load GSDATA"))
                    {
                        LoadGSDATA();
                    }
                    if (ImGui::MenuItem("Save GSDATA"))
                    {
                        SaveGSDATA();
                    }
                    if (ImGui::MenuItem("Attach to Phantom Dust"))
                    {
                        AttachToProcess();
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::Button("Options")) {
                    OptionsWindow = !OptionsWindow; // Toggle Options window
                }
                if (ImGui::BeginMenu("Window"))
                {
                    if (ImGui::MenuItem("Attack Skill Editor"))
                    {
                        AtkSkillWindow = !AtkSkillWindow; // Toggle Attack Skill Editor window
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Game"))
                {
                    if (ImGui::MenuItem("Freeze Phantom Dust"))
                    {
                        PauseGame();
                    }
                    if (ImGui::MenuItem("Unfreeze Phantom Dust"))
                    {
                        UnpauseGame();
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }
            ImGui::End();
        }

        if (ImGui::BeginViewportSideBar("StatusBar", viewport, ImGuiDir_Down, height, window_flags)) {
            if (ImGui::BeginMenuBar()) {
                string print;
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
            string WindowTitle = "Attack Skill Editor - " + filepath; // Use filename in the window title.
            ImGui::Begin(const_cast<char*>(WindowTitle.c_str()));     // TODO: chop off the filepath and use only the name.

            InputShort("Skill Text ID", &AtkSkill.SkillTextID);
            Tooltip("The skill ID from which to get the skill's name,\ndescription, etc.");
            InputShort("Register ID", &AtkSkill.RegisterID);
            Tooltip("The \"register\" / \"library\" of skills this skill\nwill belong to.");
            InputShort("Skill ID", &AtkSkill.SkillID);
            Tooltip("The skill's internal ID. This will determine what\nskill (if any) will be overwritten. This ID has no\nrelation to the IDs seen in-game.");

            char* rarity_txt[] = { "1","2","3","4","5" };
            AtkSkill.RarityStars = ComboShort("Rarity", rarity_txt, 5);
            Tooltip("The skill's in-game rarity, displayed as stars.");

            InputShort("Sound File ID", &AtkSkill.SoundFileID);
            InputShort("Capsule Type", &AtkSkill.CapsuleType);

            // char* capsule_types[] = {"Aura Particle","Attack","Defense","Erase","Environmental","Status","Special"};
            // ImGui::Combo("Capsule Type", &Idx, capsule_types, IM_ARRAYSIZE(capsule_types), IM_ARRAYSIZE(capsule_types));
            Tooltip("The skill's type. (Attack, Defense, Environmental, etc.)");

            InputShort("School ID", &AtkSkill.SchoolID);
            Tooltip("The skill's school. (Nature, Optical, Ki, etc.)");
            InputShort("Animation Profile (Ground)", &AtkSkill.AnimationProfileGround);
            Tooltip("This controls the player's skeletal animation, the number\nof projectiles fired, particle effects used, and much more.\nThis profile is used when the skill is cast on the ground.");
            InputShort("Animation Profile (Air)", &AtkSkill.AnimationProfileAir);
            Tooltip("This controls the player's skeletal animation, the number\nof projectiles fired, particle effects used, and much more.\nThis profile is used when the skill is cast in the air.");
            InputShort("Multi Press 1", &AtkSkill.MultiPress1);
            InputShort("Multi Press 2", &AtkSkill.MultiPress2);
            InputShort("Double Skill 1", &AtkSkill.DoubleSkill1);
            InputShort("Double Skill 2", &AtkSkill.DoubleSkill2);
            InputShort("After Hit SFX", &AtkSkill.PostHitSFX);
            InputShort("Start Up SFX", &AtkSkill.StartUpSFX);
            Tooltip("The sound effect ID to be played when the skill\nis winding up / charging.");
            InputShort("Collision SFX", &AtkSkill.CollisionSFX);
            Tooltip("The sound effect ID to be played when the skill\ncollides with something.");
            InputShort("Aura Cost", &AtkSkill.Cost);
            Tooltip("The amount of Aura the skill costs.");
            InputShort("Additional Aura Cost", &AtkSkill.ExtraCost);
            Tooltip("Adds to the base Aura cost.");
            InputShort("Health Penalty", &AtkSkill.HealthCost);
            Tooltip("The amount of health to be taken from the user\nwhen the skill is cast.");
            InputShort("# of Uses", &AtkSkill.SkillUses);
            Tooltip("How many times the skill can be used. Set this\nto 0 for infinite uses.");
            InputShort("Self Effect", &AtkSkill.SelfEffect);
            InputShort("Button Restrictions", &AtkSkill.ButtonRestrictions);
            InputShort("Requirements", &AtkSkill.Requirements);
            Tooltip("The skill's type of special requirement.\n0 = None\n1 = Health\n5 = # of Skills Remaining in Deck\n7 = Aura\n9 = Level");
            InputShort("Requirement Amount", &AtkSkill.ReqAmount);
            Tooltip("The required amount of the type specified above");

            char* items[] = { "Ground","Air","Both" };
            AtkSkill.GroundAirBoth = ComboShort("Air/Ground/Both", items, IM_ARRAYSIZE(items));
            Tooltip("Whether the skill can be used on\nthe ground, in the air, or both.");

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

        if (OptionsWindow)
        {
            ImGui::Begin("Settings");
            ImGui::InputText("Phantom Dust Game Folder", PhantomDustDir, IM_ARRAYSIZE(PhantomDustDir));
            Tooltip("The folder containing PDUWP.exe. You must have a\n dumped copy of the game files to use this option.");

            ImGui::End();
        }

        if (SkillPackWindow)
        {
            ImGui::Begin("Enter a name for your skill pack: ");
            ImGui::InputText("test", packname, IM_ARRAYSIZE(packname));

            if (ImGui::Button("Save")) {
                if (SUCCEEDED(FileSaveDialog()))
                {
                    SaveSkillPack();
                    SkillPackWindow = false;
                }
            }

            ImGui::End();
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
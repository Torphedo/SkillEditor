#pragma once
#include <main.h>
#include <UI.h>
#include <editors.h>

short timer;
bool GamePaused = false;

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
                        if (ImGui::MenuItem("Skill"))
                        {
                            if (FileSelectDialog(skillfile) != -1) // Open a file open dialog
                            {
                                LoadAttackSkill();        // Loads the current file into the atkskill struct
                                cout << "Imported attack skill " << filepath << "\n";
                                AtkSkillState = 1;        // Causes a message to appear on the status bar
                                RenderAtkSkillWindow = true;    // Opens the Attack Skill Editor window
                            }
                            else
                            {
                                cout << "File selection canceled.\n";
                                ErrorCode = 1;
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
                                ErrorCode = 1;
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
                                    ErrorCode = 1;
                                }
                            }
                        }
                        ImGui::EndMenu();
                    }
                    if (ImGui::MenuItem("Save", "Ctrl + S"))
                    {
                        SafeAtkSave();
                    }
                    if (ImGui::MenuItem("Save As", "Ctrl + Shift + S"))
                    {
                        SafeAtkSaveAs();
                    }

                    if (ImGui::MenuItem("Exit", "Alt + F4"))
                    {
                        return 0;
                    }
                    ImGui::EndMenu();
                }
                if (DebugMode)
                {
                    if (ImGui::BeginMenu("Edit") && DebugMode)
                    {
                        if (ImGui::MenuItem("Load GSDATA"))
                        {
                            // LoadGSDATA();
                        }
                        if (ImGui::MenuItem("Save GSDATA"))
                        {
                            // SaveGSDATA();
                        }
                        ImGui::EndMenu();
                    }
                }
                if (DebugMode)
                {
                    if (ImGui::Button("Options"))
                    {
                        OptionsWindow = !OptionsWindow; // Toggle Options window
                    }
                }
                if (ImGui::BeginMenu("Window"))
                {
                    if (ImGui::MenuItem("Attack Skill Editor"))
                    {
                        RenderAtkSkillWindow = !RenderAtkSkillWindow; // Toggle Attack Skill Editor window
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
                    if (DebugMode)
                    {
                        if (ImGui::MenuItem("Read GSData") && DebugMode)
                        {
                            GetProcess();
                            LoadGSDataFromRAM();
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
                                ErrorCode = 1;
                            }
                        }
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }
            ImGui::End();
        }

        // Redundant AtkSkillState checks here, but it stops console spam.
        // TODO: add a timer here to prevent more console spam.
        if (timer == 0)
        {
            if (GetKeyState(VK_CONTROL) & GetKeyState('S') & 0x8000 && AtkSkillState != 0)
            {
                SafeAtkSave();
                timer = 20;
            }

            if (GetKeyState(VK_CONTROL) & GetKeyState(VK_SHIFT) & GetKeyState('S') & 0x8000 && AtkSkillState != 0)
            {
                SafeAtkSaveAs();
                timer = 20;
            }

            if (GetKeyState(VK_CONTROL) & GetKeyState('N') & 0x8000 && !RenderSkillPackWindow)
            {
                SafeNewPack();
                timer = 20;
            }
        }
        else {
            timer--;
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

        if (RenderAtkSkillWindow)
        {
            AtkSkillWindow();
        }

        if (OptionsWindow)
        {
            ImGui::Begin("Settings");
            ImGui::InputText("Phantom Dust Game Folder", PhantomDustDir, IM_ARRAYSIZE(PhantomDustDir));
            Tooltip("The folder containing PDUWP.exe. You must have a\n dumped copy of the game files to use this option.");

            ImGui::End();
        }

        if (RenderSkillPackWindow)
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
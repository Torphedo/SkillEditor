#include "winAPI.hxx"

std::string* multiselectpath;
unsigned int MultiSelectCount = 0;


void init_winapi() {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
}

char* WINAPI file_select_dialog(const COMDLG_FILTERSPEC fileTypes) {
    IFileOpenDialog* pFileOpen;
    char* selected_filepath = nullptr;

    // Create the FileOpenDialog object.
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

    if (SUCCEEDED(hr)) {
        // Show the Open dialog box.
        hr = pFileOpen->SetFileTypes(1, &fileTypes);
        hr = pFileOpen->Show(nullptr);

        // Get the file name from the dialog box.
        if (hr == 0x800704c7) { // ERROR_CANCELLED
            return nullptr;
        }
        if (SUCCEEDED(hr)) {
            IShellItem* pItem;
            hr = pFileOpen->GetResult(&pItem);
            if (SUCCEEDED(hr)) {
                PWSTR pszFilePath;
                hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                // Display the file name to the user.
                if (SUCCEEDED(hr)) {
                    // Converts the PWSTR filepath data to a char array
                    selected_filepath = new char[wcslen(pszFilePath)];
                    wcstombs(selected_filepath, pszFilePath, wcslen(pszFilePath) + 1);
                    // Can't figure out how to use wcstombs_s()...

                    CoTaskMemFree(pszFilePath);
                }
                pItem->Release();
            }
        }
        pFileOpen->Release();
    }
    return selected_filepath;
}

HRESULT file_multiple_select_dialog() {
    IFileOpenDialog* dialog = nullptr;

    // Create the dialog object.
    HRESULT result_create = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
    if (!SUCCEEDED(result_create)) {
        // Failed to create dialog.
        return result_create;
    }

    DWORD dwOptions = 0;
    const HRESULT res_get_options = dialog->GetOptions(&dwOptions);
    if (!SUCCEEDED(res_get_options)) {
        // Failed to get options.
        return res_get_options;
    }

    // Set flag for multiselect
    HRESULT res_set_options = dialog->SetOptions(dwOptions | FOS_ALLOWMULTISELECT);
    if (!SUCCEEDED(res_set_options)) {
        // Failed to set flag
        dialog->Release();
        return res_set_options;
    }

    // Show the Open dialog.
    HRESULT hr = dialog->SetDefaultExtension(L".bin");
    hr = dialog->Show(nullptr);

    if (SUCCEEDED(hr)) {
        // Obtain the result of the user interaction.
        IShellItemArray* psiaResults;
        hr = dialog->GetResults(&psiaResults);

        if (SUCCEEDED(hr)) {
            PWSTR pszFilePath = nullptr;
            DWORD dwNumItems = 0; // number of items in multiple selection
            std::wstring strSelected; // will hold file paths of selected items

            hr = psiaResults->GetCount(&dwNumItems);  // get number of selected items

            if (SUCCEEDED(hr)) {
                multiselectpath = new std::string[dwNumItems];
                // Loop through IShellItemArray and construct string for display
                for (DWORD i = 0; i < dwNumItems; i++) {
                    IShellItem* psi = nullptr;
                    MultiSelectCount = dwNumItems;

                    hr = psiaResults->GetItemAt(i, &psi); // get a selected item from the IShellItemArray

                    if (SUCCEEDED(hr)) {
                        hr = psi->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                        if (SUCCEEDED(hr)) {
                            // Converts the PWSTR filepath data to a char array, which is then written to a string
                            char* temp = new char[wcslen(pszFilePath)];
                            wcstombs(temp, pszFilePath, wcslen(pszFilePath) + 1);
                            // Can't figure out how to use wcstombs_s()...

                            multiselectpath[i] = temp;
                            CoTaskMemFree(pszFilePath);
                        }

                        psi->Release();
                    }
                }
            }
        }
    }
    dialog->Release();
    return hr;
}

char* WINAPI file_save_dialog(const COMDLG_FILTERSPEC fileTypes, LPCWSTR DefaultExtension) {
    IFileSaveDialog* pFileOpen = nullptr;
    char* selected_filepath = nullptr;

    // Create the dialog
    HRESULT res_create = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_ALL, IID_IFileSaveDialog, reinterpret_cast<void**>(&pFileOpen));
    if (!SUCCEEDED(res_create)) {
        // Failed to create the dialog
        return nullptr;
    }

    // Show the Open dialog box.
    HRESULT hr = pFileOpen->SetFileTypes(1, &fileTypes);
    hr = pFileOpen->SetDefaultExtension(DefaultExtension);
    hr = pFileOpen->Show(nullptr);

    // Get the file name from the dialog box.
    if (hr == 0x800704c7) { // ERROR_CANCELLED
        return nullptr;
    }
    if (SUCCEEDED(hr)) {
        IShellItem* pItem;
        hr = pFileOpen->GetResult(&pItem);
        if (SUCCEEDED(hr)) {
            PWSTR pszFilePath;
            hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

            // Display the file name to the user.
            if (SUCCEEDED(hr)) {
                // Converts the PWSTR filepath data to a char array
                selected_filepath = new char[wcslen(pszFilePath)];
                wcstombs(selected_filepath, pszFilePath, wcslen(pszFilePath) + 1);
                // Can't figure out how to use wcstombs_s()...

                CoTaskMemFree(pszFilePath);
            }
            pItem->Release();
        }
    }

    pFileOpen->Release();
    return selected_filepath;
}

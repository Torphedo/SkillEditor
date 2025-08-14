#include "winAPI.hxx"

std::string* multiselectpath;
unsigned int MultiSelectCount = 0;

HRESULT hr;

void init_winapi() {
    hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
}

HRESULT file_multiple_select_dialog() {
    IFileOpenDialog* pfd;

    // CoCreate the dialog object.
    hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));

    if (SUCCEEDED(hr)) {
        DWORD dwOptions;
        // Specify multiselect.
        hr = pfd->GetOptions(&dwOptions);

        if (SUCCEEDED(hr)) {
            hr = pfd->SetOptions(dwOptions | FOS_ALLOWMULTISELECT);
        }

        if (SUCCEEDED(hr)) {
            // Show the Open dialog.
            hr = pfd->SetDefaultExtension(L".bin");
            hr = pfd->Show(nullptr);

            if (SUCCEEDED(hr)) {
                // Obtain the result of the user interaction.
                IShellItemArray* psiaResults;
                hr = pfd->GetResults(&psiaResults);

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
        }
        pfd->Release();
    }
    return hr;
}
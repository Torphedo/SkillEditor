#include "winAPI.h"

char* selected_filepath;
std::string* multiselectpath;
int MultiSelectCount = 0;

HRESULT hr;

int WINAPI FileSelectDialog(const COMDLG_FILTERSPEC fileTypes)
{
    IFileOpenDialog* pFileOpen;

    // Create the FileOpenDialog object.
    hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

    if (SUCCEEDED(hr))
    {
        // Show the Open dialog box.
        hr = pFileOpen->SetFileTypes(1, &fileTypes);
        hr = pFileOpen->Show(NULL);

        // Get the file name from the dialog box.
        if (hr == 0x800704c7) // ERROR_CANCELLED
        {
            return -1;
        }
        if (SUCCEEDED(hr))
        {
            IShellItem* pItem;
            hr = pFileOpen->GetResult(&pItem);
            if (SUCCEEDED(hr))
            {
                PWSTR pszFilePath;
                hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                // Display the file name to the user.
                if (SUCCEEDED(hr))
                {
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
    return 0;
}

HRESULT MultiSelectWindow()
{
    IFileOpenDialog* pfd;

    // CoCreate the dialog object.
    hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));

    if (SUCCEEDED(hr))
    {
        DWORD dwOptions;
        // Specify multiselect.
        hr = pfd->GetOptions(&dwOptions);

        if (SUCCEEDED(hr))
        {
            hr = pfd->SetOptions(dwOptions | FOS_ALLOWMULTISELECT);
        }

        if (SUCCEEDED(hr))
        {
            // Show the Open dialog.
            hr = pfd->SetDefaultExtension(L".bin");
            hr = pfd->Show(NULL);

            if (SUCCEEDED(hr))
            {
                // Obtain the result of the user interaction.
                IShellItemArray* psiaResults;
                hr = pfd->GetResults(&psiaResults);

                if (SUCCEEDED(hr))
                {
                    PWSTR pszFilePath = NULL;
                    DWORD dwNumItems = 0; // number of items in multiple selection
                    std::wstring strSelected; // will hold file paths of selected items

                    hr = psiaResults->GetCount(&dwNumItems);  // get number of selected items

                    if (SUCCEEDED(hr))
                    {
                        multiselectpath = new std::string[dwNumItems];
                        // Loop through IShellItemArray and construct string for display
                        for (DWORD i = 0; i < dwNumItems; i++)
                        {
                            IShellItem* psi = NULL;
                            MultiSelectCount = dwNumItems;

                            hr = psiaResults->GetItemAt(i, &psi); // get a selected item from the IShellItemArray

                            if (SUCCEEDED(hr))
                            {
                                hr = psi->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                                if (SUCCEEDED(hr))
                                {
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

int WINAPI FileSaveDialog(const COMDLG_FILTERSPEC fileTypes, LPCWSTR DefaultExtension)
{
    IFileSaveDialog* pFileOpen;

    // Create the FileOpenDialog object.
    hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL, IID_IFileSaveDialog, reinterpret_cast<void**>(&pFileOpen));

    if (SUCCEEDED(hr))
    {
        // Show the Open dialog box.
        hr = pFileOpen->SetFileTypes(1, &fileTypes);
        hr = pFileOpen->SetDefaultExtension(DefaultExtension);
        hr = pFileOpen->Show(NULL);

        // Get the file name from the dialog box.
        if (hr == 0x800704c7) // ERROR_CANCELLED
        {
            return -1;
        }
        if (SUCCEEDED(hr))
        {
            IShellItem* pItem;
            hr = pFileOpen->GetResult(&pItem);
            if (SUCCEEDED(hr))
            {
                PWSTR pszFilePath;
                hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                // Display the file name to the user.
                if (SUCCEEDED(hr))
                {
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
    return 0;
}

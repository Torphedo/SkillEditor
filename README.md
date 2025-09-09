Skill Editor is a live modding tool for the Windows 10 port of Phantom Dust (2004). It only edits the GSDATA region,
which contains all the skills/cards/moves (whatever you want to call them), and all their relevant text.

<img width="1265" height="636" alt="2025-09-08 23_25_01-Paint 3D" src="https://github.com/user-attachments/assets/6c4f83cf-acf9-46cf-bd25-5d838cc25b28" />
<img width="1280" height="691" alt="2025-09-08 23_24_10-" src="https://github.com/user-attachments/assets/aa62fc1b-9222-4f96-a03f-ee28a1cac856" />

# Labels
The main thing Skill Editor offers is a (slightly more) human friendly way to edit skill data. The UI exposes bytes
of skill data with named input fields called labels. For people comfortable with hex editors, there's also a built-in
hex editor window that shows you only the current skill data.

In Phantom Dust, the
meaning of each byte in the skill data can drastically change depending on the numbers in the first half of the skill.
Because of this, all labels are configurable (in `labels.yaml`) and can change depending on other data in the skill.

# Modpacks & Distribution
You can save individual skills as `.sp3` files, using the `File > Save to File` or `File > Save As` options. `Save As` always asks you where to save
the skill, whereas `Save to File` is like a standard `Save` button and overwrites the last file you saved.
Once you have multiple skills saved, you can merge them into a single skill pack file (also named `.sp3`, because individual
skills are considered 1-skill modpacks). To make one, click `File > New Skill Pack`, select some SP3 files, and pick a filename.

Single skill files are under 200 bytes, and the largest modpacks so far are well under 100KiB.
These SP3 files are also supported by Nuion's tool [PD Helper](https://github.com/eradication0/PDHelper), which is also
open-source.

## Existing Modpacks
- Ash2Dust [[files](https://drive.google.com/drive/folders/1XtTEODxI77dZlFXApJ3HbC_m6yTxUvCl)] [[changelog](https://docs.google.com/document/d/12RotoegEfNXorSlYx6NXxf-au-kOZJDmcuHmFZv0Fb4/edit?usp=drivesdk)]
  - The JSON file has arsenals/decks tailored for the modpack, for use with PD Helper.

# Troubleshooting
Skill Editor tries to detect when the game reboots, but sometimes it still gets out of sync. This can also happen if you run Skill
Editor as the game is booting, before it's had a chance to actually load GSDATA. If you see all zeroes or something is acting
strange, try the `Refresh process` button.

If the text boxes in the text editor window are empty even after the game is running, press `Reload`.

For more info on how to write custom labels, look at `labels.yaml` or read the documentation in the documentation window of the editor.
All the information here is also written there.

## Known Issues
- Occasional "Failed to read data from Phantom Dust (error code 299) errors
I don't know what causes this to happen, on my machine they only happen while the game is starting and are fixed by hitting `Refresh process`.
For some other people, they happen consistently and completely break the editor.

# How?
The editor uses [`ReadProcessMemory`](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-readprocessmemory)
to make a copy of the game's skill and text data, then uses [`GetWriteWatch`](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-getwritewatch)
and [`WriteProcessMemory`](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-writeprocessmemory)
to automatically sync with the game whenever it changes (see `src/remote_pd.c`).

This happens every frame, so skill and text data is always synced as fast as it updates in the editor UI. The rest of the
editor code is pretty straightforward, since synchronization is completely automatic.

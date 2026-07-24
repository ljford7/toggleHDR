/*
    toggleHDR
    Copyright (C) 2025  Clayton Macleod

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <windows.h>
#pragma comment(lib, "User32.lib")
#include <shellapi.h>
#pragma comment(lib, "Shell32.lib")
#include <string.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    int open_settings = 1;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--no-icc") == 0) {
            open_settings = 0;
        }
    }

    UINT32 pathCount = 0, modeCount = 0;
    if (GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &pathCount, &modeCount) != ERROR_SUCCESS) return 1;

    DISPLAYCONFIG_PATH_INFO* paths = new DISPLAYCONFIG_PATH_INFO[pathCount];
    DISPLAYCONFIG_MODE_INFO* modes = new DISPLAYCONFIG_MODE_INFO[modeCount];

    if (QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &pathCount, paths, &modeCount, modes, nullptr) != ERROR_SUCCESS) {
        delete[] paths; delete[] modes; return 1;
    }

    BOOL toggledAny = FALSE;

    // Iterate through all active display paths
    for (UINT32 i = 0; i < pathCount; ++i) {
        const auto& t = paths[i].targetInfo;

        DISPLAYCONFIG_GET_ADVANCED_COLOR_INFO g = {};
        g.header.type = static_cast<DISPLAYCONFIG_DEVICE_INFO_TYPE>(DISPLAYCONFIG_DEVICE_INFO_GET_ADVANCED_COLOR_INFO);
        g.header.size = sizeof(g);
        g.header.adapterId = t.adapterId;
        g.header.id = t.id;

        if (DisplayConfigGetDeviceInfo(&g.header) != ERROR_SUCCESS)
            continue;

        // Skip displays that don't support advanced color / HDR
        if (!g.advancedColorSupported)
            continue;

        const BOOL turnOn = g.advancedColorEnabled ? FALSE : TRUE;

#if defined(DISPLAYCONFIG_DEVICE_INFO_SET_HDR_STATE) && defined(DISPLAYCONFIG_SET_HDR_STATE)
        {
            DISPLAYCONFIG_SET_HDR_STATE s = {};
            s.header.type = static_cast<DISPLAYCONFIG_DEVICE_INFO_TYPE>(DISPLAYCONFIG_DEVICE_INFO_SET_HDR_STATE);
            s.header.size = sizeof(s);
            s.header.adapterId = t.adapterId;
            s.header.id = t.id;
            s.enableHdr = turnOn ? 1u : 0u;
            if (DisplayConfigSetDeviceInfo(&s.header) == ERROR_SUCCESS) {
                toggledAny = TRUE;
            }
        }
#else
        {
            DISPLAYCONFIG_SET_ADVANCED_COLOR_STATE s = {};
            s.header.type = static_cast<DISPLAYCONFIG_DEVICE_INFO_TYPE>(DISPLAYCONFIG_DEVICE_INFO_SET_ADVANCED_COLOR_STATE);
            s.header.size = sizeof(s);
            s.header.adapterId = t.adapterId;
            s.header.id = t.id;
            s.enableAdvancedColor = turnOn ? 1u : 0u;
            if (DisplayConfigSetDeviceInfo(&s.header) == ERROR_SUCCESS) {
                toggledAny = TRUE;
            }
        }
#endif
    }

    // Briefly launch settings once to force ICC profile reload for all displays
    if (open_settings && toggledAny) {
        Sleep(2000);
        ShellExecuteA(nullptr, "open", "ms-settings:display", nullptr, nullptr, SW_SHOWNORMAL);
        Sleep(1000);
        system("taskkill /IM SystemSettings.exe /F >nul 2>&1");
    }

    delete[] paths;
    delete[] modes;
    return 0;
}

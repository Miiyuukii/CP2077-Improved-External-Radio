mod = {
    ready = false,
    bridge = nil
}

registerForEvent("onInit", function()
    local nativeSettings = GetMod("nativeSettings")
    if not nativeSettings then
        print("[ImpExRad] Native Settings UI not installed.")
        return
    end

    nativeSettings.addTab("/ImpExRad", "CP2077 Improved External Radio")
    mod.bridge = ImpExRad.new()

    nativeSettings.addSubcategory("/ImpExRad/Devices", "Device Options")

    local rawDevices = mod.bridge:GetDevicesList() or {}
    local devicesList = {}

    for _, dev in ipairs(rawDevices) do
        if dev and dev ~= "" then
            table.insert(devicesList, dev)
        end
    end

    if #devicesList == 0 then
        devicesList = { "No Devices Found" }
    end

    nativeSettings.addSwitch(
        "/ImpExRad/Devices",
        "Use specific devices",
        "Mute specific devices app volume.",
        false,
        false,
        function(state)
            mod.bridge:SetUseDevice(state)
        end
    )

    nativeSettings.addSelectorString(
        "/ImpExRad/Devices",
        "Target Audio Device",
        "Select the Windows playback device used for external audio.",
        devicesList,
        1,
        1,
        function(selectedValue)
            if type(selectedValue) == "number" then
                if devicesList[selectedValue] then
                    mod.bridge:SetDevice(devicesList[selectedValue])
                end
            elseif type(selectedValue) == "string" then
                mod.bridge:SetDevice(selectedValue)
            end
        end
    )

    nativeSettings.addButton(
        "/ImpExRad/Devices",
        "Refresh Audio Devices",
        "Rescan active Windows audio devices.",
        "Refresh",
        45,
        function()
            mod.bridge:ReloadDevices()
        end
    )

    nativeSettings.addSubcategory("/ImpExRad/App", "App Settings")
    local apps = {
        [1] = "Spotify",
        [2] = "Google Chrome",
        [3] = "Firefox",
        [4] = "Microsoft Edge",
        [5] = "VLC media player",
        [6] = "foobar2000",
        [7] = "Apple Music",
        [8] = "Tidal",
        [9] = "MusicBee",
        [10] = "Windows Media Player",
        [11] = "AIMP",
        [12] = "Opera",
        [13] = "Brave",
        [14] = "Discord"
    }

    nativeSettings.addSwitch(
        "/ImpExRad/App",
        "Use specific app",
        "Mute specific app app volume.",
        false,
        false,
        function(state)
            mod.bridge:SetUseApp(state)
        end
    )

    local initialApp = mod.bridge:GetAppIndex() or 0
    local currentAppIndex = math.min(math.max(initialApp + 1, 1), #apps)
    nativeSettings.addSelectorString(
        "/ImpExRad/App",
        "Applications",
        "Choose which applications will be affected by behavior.",
        apps,
        1,               -- Default index
        currentAppIndex, -- Initial active index
        function(idx)
            local targetIdx = currentAppIndex
            if type(idx) == "number" then
                targetIdx = idx
            elseif type(idx) == "string" then
                for i, name in ipairs(apps) do
                    if name == idx then
                        targetIdx = i
                        break
                    end
                end
            end
        end
    )

    nativeSettings.addSubcategory("/ImpExRad/Behavior", "Options")

    local modes = {
        [1] = "Pause Media on Exit",
        [2] = "Mute Volume on Exit",
        [3] = "Disabled (Do Nothing)"
    }

    -- Convert 0-based C++ enum/index to 1-based Lua index
    local initialMode = mod.bridge:GetMode() or 0
    local currentModeIndex = math.min(math.max(initialMode + 1, 1), #modes)

    nativeSettings.addSelectorString(
        "/ImpExRad/Behavior",
        "Behavior Mode",
        "Choose how external audio behaves when exiting or entering a vehicle.",
        modes,
        1,                -- Default index
        currentModeIndex, -- Initial active index
        function(idx)
            local modeValue = (type(idx) == "number" and idx or currentModeIndex) - 1
            mod.bridge:SetMode(modeValue)
        end
    )

    mod.ready = true
end)

return mod

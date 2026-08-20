mod = {
    ready = false,
    bridge = nil
}

modSettings = {
    useDevice = false,
    deviceIndex = 1,
    useApp = false,
    appIndex = 1,
    behaviorMode = 1,
}

function SaveSettings()
    local f = io.open("settings.json", "w")
    if f then
        local ms = json.encode(modSettings)
        f:write(ms)
        f:close()
    end
end

function LoadSettings()
    if SettingsExist() then
        local f = io.open("settings.json", "r")
        if f then
            modSettings = json.decode(f:read("*a"))
            f:close()
        end
    end
end

function SettingsExist()
    local f = io.open("settings.json", "r")
    if f then
        f:close()
        return true
    end
    return false
end

registerForEvent("onInit", function()
    local nativeSettings = GetMod("nativeSettings")
    if not nativeSettings then
        print("[ImpExRad] Native Settings UI not installed.")
        return
    end

    nativeSettings.addTab("/ImpExRad", "CP2077 Improved External Radio")
    mod.bridge = ImpExRad.new()

    if SettingsExist() then
        LoadSettings()
    end

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
        modSettings.useDevice,
        false,
        function(state)
            mod.bridge:SetUseDevice(state)
            modSettings.useDevice = state
            SaveSettings()
        end
    )

    local dIndex = 1;
    if #devicesList >= modSettings.deviceIndex then
        dIndex = modSettings.deviceIndex
    end

    nativeSettings.addSelectorString(
        "/ImpExRad/Devices",
        "Target Audio Device",
        "Select the Windows playback device used for external audio.",
        devicesList,
        dIndex,
        1,
        function(selectedValue)
            mod.bridge:SetDevice(devicesList[selectedValue])
            modSettings.deviceIndex = selectedValue
            SaveSettings()
        end
    )
    --[[
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
        ]] --

    nativeSettings.addSubcategory("/ImpExRad/App", "App Options")
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
        modSettings.useApp,
        false,
        function(state)
            mod.bridge:SetUseApp(state)
            modSettings.useApp = state;
            SaveSettings()
        end
    )

    local aIndex = 1;
    if #apps >= modSettings.appIndex then
        aIndex = modSettings.appIndex
    end
    nativeSettings.addSelectorString(
        "/ImpExRad/App",
        "Applications",
        "Choose which applications will be affected by behavior.",
        apps,
        aIndex,
        1,
        function(idx)
            mod.bridge:SetAppIndex(idx - 1)
            modSettings.appIndex = idx
            SaveSettings()
        end
    )

    nativeSettings.addSubcategory("/ImpExRad/Behavior", "Options")

    local modes = {
        [1] = "Pause Media on Exit",
        [2] = "Mute Volume on Exit",
        [3] = "Disabled (Do Nothing)"
    }

    nativeSettings.addSelectorString(
        "/ImpExRad/Behavior",
        "Behavior Mode",
        "Choose how external audio behaves when exiting or entering a vehicle.",
        modes,
        modSettings.behaviorMode,
        1,
        function(idx)
            mod.bridge:SetMode(idx - 1)
            modSettings.behaviorMode = idx
            SaveSettings()
        end
    )

    mod.ready = true
end)

return mod

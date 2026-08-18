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
    -- Instantiate the RTTI class registered by your RED4ext DLL
    mod.bridge = ImpExRad.new()

    nativeSettings.addSubcategory("/ImpExRad/Devices", "Device Options")

    -- Fetch devices list safely
    local rawDevices = mod.bridge:GetDevicesList() or {}
    local devicesList = {}

    -- Ensure array has valid, non-empty string elements
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

    -- Add Device Selector Dropdown
    nativeSettings.addSelectorString(
        "/ImpExRad/Devices",
        "Target Audio Device",
        "Select the Windows playback device used for external audio.",
        devicesList,
        1, -- Default selection
        1, -- Current selection
        function(selectedValue)
            -- Handles both Index (int) or Direct String passing depending on Native Settings version
            if type(selectedValue) == "number" then
                if devicesList[selectedValue] then
                    mod.bridge:SetDevice(devicesList[selectedValue])
                end
            elseif type(selectedValue) == "string" then
                mod.bridge:SetDevice(selectedValue)
            end
        end
    )

    -- Refresh Devices Button
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
    local apps = {}
    local initialMode = mod.bridge:GetAppIndex() or 0
    local currentAppIndex = math.min(math.max(initialMode + 1, 1), #apps)
    nativeSettings.addSelectorString(
        "/ImpExRad/App",
        "Applications",
        "Choose which applications will be affected by behavior.",
        apps,
        1,               -- Default index
        currentAppIndex, -- Initial active index
        function(idx)
            local appValue = (type(idx) == "number" and idx or currentAppIndex) - 1
            mod.bridge:Set(appValue)
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

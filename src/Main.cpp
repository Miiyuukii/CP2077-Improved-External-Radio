#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Containers/DynArray.hpp>
#include <RED4ext/CString.hpp>

// Fetch audio devices on Windows.
#include <iostream>
#include <windows.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <endpointvolume.h>
#include <functiondiscoverykeys_devpkey.h>
#include <vector>
#include <string>

// Pause/Play functionality
#pragma comment(lib, "runtimeobject.lib")
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.Control.h>
#include <thread>

// Target specific apps
#include <psapi.h>
#include <algorithm>

static std::vector<std::string> devices = {};
static std::vector<std::string> guids = {};

static std::string currGuid = "";

static int32_t mode = 0; // 0 = pause when out of car, 1 = mute when out of car, 2 = don't do anything

static bool useDevice = false;
static bool useAppName = false;

static std::vector<std::string> apps = {
    "Spotify.exe", "chrome.exe",   "firefox.exe",  "msedge.exe", "vlc.exe",   "foobar2000.exe", "AppleMusic.exe",
    "TIDAL.exe",   "MusicBee.exe", "wmplayer.exe", "AIMP.exe",   "opera.exe", "brave.exe",      "Discord.exe"
};
static int32_t appIndex = 0;

static float volume = 1.0;

static RED4ext::v1::PluginHandle g_pluginHandle = nullptr;
static const RED4ext::v1::Sdk* g_sdk = nullptr;

using namespace winrt::Windows::Media::Control;
void PauseMediaAsync()
{
    std::thread(
        []()
        {
            try
            {
                winrt::init_apartment(winrt::apartment_type::multi_threaded);

                auto manager = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
                auto currentSession = manager.GetCurrentSession();

                if (currentSession)
                {
                    currentSession.TryPauseAsync().get();
                }
            }
            catch (...)
            {
                
            }
        })
        .detach();
}
void ResumeMediaAsync()
{
    std::thread(
        []()
        {
            try
            {
                winrt::init_apartment(winrt::apartment_type::multi_threaded);

                auto manager = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
                auto currentSession = manager.GetCurrentSession();

                if (currentSession)
                {
                    currentSession.TryPlayAsync().get();
                }
            }
            catch (...)
            {
            }
        })
        .detach();
}

void GetDevicesList(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame,
                    RED4ext::DynArray<RED4ext::CString>* aOut, int64_t a4)
{
    if (aOut)
    {
        aOut->Clear();
        aOut->Reserve(static_cast<uint32_t>(::devices.size()));
        for (const auto& dev : ::devices)
        {
            aOut->PushBack(dev.c_str());
        }
    }
}

static std::string WideToUTF8(const wchar_t* wstr)
{
    if (!wstr)
        return "";
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    if (sizeNeeded <= 0)
        return "";

    std::string strTo(sizeNeeded - 1, 0); // Exclude null terminator from std::string length
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &strTo[0], sizeNeeded, NULL, NULL);
    return strTo;
}
static std::wstring UTF8ToWide(const std::string& str)
{
    if (str.empty())
        return L"";
    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    if (sizeNeeded <= 0)
        return L"";

    std::wstring wstr(sizeNeeded - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], sizeNeeded);
    return wstr;
}
static void ReloadDevicesInternal()
{
    ::devices.clear();
    ::guids.clear();

    HRESULT hrCo = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    bool shouldUninit = SUCCEEDED(hrCo);

    IMMDeviceEnumerator* pEnumerator = nullptr;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                                  (void**)&pEnumerator);

    if (SUCCEEDED(hr) && pEnumerator)
    {
        IMMDeviceCollection* pCollection = nullptr;
        hr = pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pCollection);

        if (SUCCEEDED(hr) && pCollection)
        {
            UINT count = 0;
            pCollection->GetCount(&count);

            for (UINT i = 0; i < count; i++)

            {
                IMMDevice* pEndpoint = nullptr;
                if (SUCCEEDED(pCollection->Item(i, &pEndpoint)) && pEndpoint)

                {
                    // 1. Get Endpoint ID String (GUID)
                    LPWSTR pwszID = nullptr;
                    std::string deviceGuid = "";
                    if (SUCCEEDED(pEndpoint->GetId(&pwszID)) && pwszID)
                    {
                        deviceGuid = WideToUTF8(pwszID);
                        CoTaskMemFree(pwszID);
                    }

                    // 2. Get Device Friendly Name
                    std::string deviceName = "";
                    IPropertyStore* pProps = nullptr;
                    if (SUCCEEDED(pEndpoint->OpenPropertyStore(STGM_READ, &pProps)) && pProps)
                    {
                        PROPVARIANT varName;
                        PropVariantInit(&varName);

                        if (SUCCEEDED(pProps->GetValue(PKEY_Device_FriendlyName, &varName)))

                        {
                            if (varName.vt == VT_LPWSTR && varName.pwszVal != nullptr)

                            {
                                deviceName = WideToUTF8(varName.pwszVal);
                            }
                        }
                        PropVariantClear(&varName);
                        pProps->Release();
                    }

                    // Store synchronized pair
                    if (!deviceGuid.empty() && !deviceName.empty())
                    {
                        ::devices.push_back(deviceName);
                        ::guids.push_back(deviceGuid);
                    }

                    pEndpoint->Release();
                }
            }
            pCollection->Release();
        }
        pEnumerator->Release();
    }

    if (shouldUninit)
    {
        CoUninitialize();
    }
}

void ReloadDevices(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, void* aOut, int64_t a4)
{
    ReloadDevicesInternal();
}
void SetDevice(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, void* aOut, int64_t a4)
{
    RED4ext::CString newDevice;
    RED4ext::GetParameter(aFrame, &newDevice);
    aFrame->code++;

    std::string targetName = newDevice.c_str();

    for (size_t i = 0; i < ::devices.size(); ++i)
    {
        if (::devices[i] == targetName && i < ::guids.size())
        {
            ::currGuid = ::guids[i];
            break;
        }
    }
}

static std::string GetProcessNameFromPID(DWORD pid)
{
    if (pid == 0)
        return "";

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProcess)
        return "";

    wchar_t buffer[MAX_PATH] = {0};
    DWORD size = MAX_PATH;
    std::string processName = "";

    if (QueryFullProcessImageNameW(hProcess, 0, buffer, &size))
    {
        std::wstring fullPath(buffer);
        size_t lastSlash = fullPath.find_last_of(L"\\/");
        std::wstring fileName = (lastSlash != std::wstring::npos) ? fullPath.substr(lastSlash + 1) : fullPath;
        processName = WideToUTF8(fileName.c_str());
    }

    CloseHandle(hProcess);
    return processName;
}
static bool EqualsIgnoreCase(const std::string& a, const std::string& b)
{
    if (a.size() != b.size())
        return false;
    return std::equal(
        a.begin(), a.end(), b.begin(), [](char ca, char cb)
        { return std::tolower(static_cast<unsigned char>(ca)) == std::tolower(static_cast<unsigned char>(cb)); });
}
static void SetCurrentSessionVolumeInternal(float volumeLevel)
{
    float clampedVol = (volumeLevel < 0.0f) ? 0.0f : ((volumeLevel > 1.0f) ? 1.0f : volumeLevel);
    bool isMuteRequested = (clampedVol <= 0.001f);

    try
    {
        winrt::init_apartment(winrt::apartment_type::multi_threaded);
        {
            IMMDeviceEnumerator* pEnumerator = nullptr;
            if (SUCCEEDED(CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
                                           __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator)) &&
                pEnumerator)
            {
                IMMDevice* pDevice = nullptr;
                HRESULT hrDevice = E_FAIL;

                if (useDevice && !currGuid.empty())
                {
                    std::wstring wGuid = UTF8ToWide(currGuid);
                    hrDevice = pEnumerator->GetDevice(wGuid.c_str(), &pDevice);
                }

                if (FAILED(hrDevice) || !pDevice)
                {
                    hrDevice = pEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &pDevice);
                }

                if (SUCCEEDED(hrDevice) && pDevice)
                {
                    IAudioSessionManager2* pSessionManager = nullptr;
                    if (SUCCEEDED(pDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, NULL,
                                                    (void**)&pSessionManager)) &&
                        pSessionManager)
                    {
                        IAudioSessionEnumerator* pSessionEnumerator = nullptr;
                        if (SUCCEEDED(pSessionManager->GetSessionEnumerator(&pSessionEnumerator)) && pSessionEnumerator)
                        {
                            int count = 0;
                            pSessionEnumerator->GetCount(&count);

                            std::string targetApp = "";
                            std::string targetAppBase = "";
                            bool filterApp =
                                useAppName && (appIndex >= 0 && appIndex < static_cast<int32_t>(apps.size()));
                            if (filterApp)
                            {
                                targetApp = apps[appIndex];
                                targetAppBase = targetApp;
                                size_t exePos = targetAppBase.find(".exe");
                                if (exePos != std::string::npos)
                                {
                                    targetAppBase = targetAppBase.substr(0, exePos);
                                }
                            }

                            for (int i = 0; i < count; i++)
                            {
                                IAudioSessionControl* pControl = nullptr;
                                if (SUCCEEDED(pSessionEnumerator->GetSession(i, &pControl)) && pControl)
                                {
                                    bool shouldApplyVolume = true;

                                    if (filterApp)
                                    {
                                        shouldApplyVolume = false;
                                        IAudioSessionControl2* pControl2 = nullptr;

                                        if (SUCCEEDED(pControl->QueryInterface(__uuidof(IAudioSessionControl2),
                                                                               (void**)&pControl2)) &&
                                            pControl2)
                                        {
                                            // g_sdk->logger->InfoF(g_pluginHandle, "Check application.");

                                            DWORD pid = 0;
                                            pControl2->GetProcessId(&pid);

                                            std::string procName = GetProcessNameFromPID(pid);

                                            if (g_sdk && g_pluginHandle)
                                            {
                                                std::string result = procName + ": ID" + std::to_string(pid);
                                                // g_sdk->logger->InfoF(g_pluginHandle, targetApp.c_str());
                                                // g_sdk->logger->InfoF(g_pluginHandle, targetAppBase.c_str());
                                                // g_sdk->logger->InfoF(g_pluginHandle, result.c_str());
                                            }

                                            if (!procName.empty() && (EqualsIgnoreCase(procName, targetApp) ||
                                                                      procName.find(targetAppBase) != std::string::npos))
                                            {
                                                // g_sdk->logger->InfoF(g_pluginHandle, "Found application.");
                                                shouldApplyVolume = true;
                                            }
                                            else
                                            {
                                                // g_sdk->logger->InfoF(g_pluginHandle, "Application not found. Match display name..");

                                                LPWSTR pwszDisplayName = nullptr;
                                                if (SUCCEEDED(pControl2->GetDisplayName(&pwszDisplayName)) &&
                                                    pwszDisplayName)
                                                {
                                                    std::string dispName = WideToUTF8(pwszDisplayName);
                                                    CoTaskMemFree(pwszDisplayName);

                                                    if (!dispName.empty() && (EqualsIgnoreCase(dispName, targetApp) ||
                                                        dispName.find(targetApp) != std::string::npos))
                                                    {
                                                        // g_sdk->logger->InfoF(g_pluginHandle, "Found application.");
                                                        shouldApplyVolume = true;
                                                    }
                                                }
                                            }

                                            pControl2->Release();
                                        }
                                    }

                                    if (shouldApplyVolume)
                                    {
                                        ISimpleAudioVolume* pVolume = nullptr;
                                        if (SUCCEEDED(pControl->QueryInterface(__uuidof(ISimpleAudioVolume),
                                                                               (void**)&pVolume)) &&
                                            pVolume)
                                        {
                                            // g_sdk->logger->InfoF(g_pluginHandle, "Apply volume.");
                                            pVolume->SetMasterVolume(clampedVol, NULL);
                                            pVolume->SetMute(isMuteRequested ? TRUE : FALSE, NULL);
                                            pVolume->Release();
                                        }
                                    }

                                    pControl->Release();
                                }
                            }
                            pSessionEnumerator->Release();
                        }
                        pSessionManager->Release();
                    }
                    pDevice->Release();
                }
                pEnumerator->Release();
            }
        }
        winrt::uninit_apartment();
    }
    catch (...)
    {
    }
}

void SetCurrentMediaVolumeAsync(float volumeLevel)
{
    std::thread([volumeLevel]() { SetCurrentSessionVolumeInternal(volumeLevel); }).detach();
}

void SetMediaVolume(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, void* aOut, int64_t a4)
{
    float newVolume;
    RED4ext::GetParameter(aFrame, &newVolume);
    aFrame->code++;

    ::volume = newVolume;

    if (g_sdk && g_pluginHandle)
    {
        // g_sdk->logger->InfoF(g_pluginHandle, "Setting current media session volume to: %.2f", newVolume);
        // g_sdk->logger->InfoF(g_pluginHandle, "Current media session volume is %.2f", ::volume);
    }

    SetCurrentMediaVolumeAsync(newVolume);
}

void SetMode(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, void* aOut, int64_t a4)
{
    int newMode;
    RED4ext::GetParameter(aFrame, &newMode);
    aFrame->code++;

    ::mode = newMode;
}
void GetMode(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, int32_t* aOut, int64_t a4) 
{
    if (aOut)
    {
        *aOut = ::mode;
    }
}

void PauseMedia(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, void* aOut, int64_t a4)
{
    // pause current media using
    PauseMediaAsync();
}
void ResumeMedia(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, void* aOut, int64_t a4)
{
    // resume current media
    ResumeMediaAsync();
}

void SetUseDevice(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, void* aOut, int64_t a4)
{
    bool val;
    RED4ext::GetParameter(aFrame, &val);
    aFrame->code++;

    ::useDevice = val;
}

void SetUseApp(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, void* aOut, int64_t a4)
{
    bool val;
    RED4ext::GetParameter(aFrame, &val);
    aFrame->code++;

    ::useAppName = val;
}

void GetAppIndex(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, int32_t* aOut, int64_t a4)
{
    if (aOut)
    {
        *aOut = ::appIndex;
    }
}
void SetAppIndex(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, void* aOut, int64_t a4)
{
    int32_t val;
    RED4ext::GetParameter(aFrame, &val);
    aFrame->code++;

    ::appIndex = val;
}

void GetVolume(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, float* aOut, int64_t a4)
{
    if (aOut)
    {
        *aOut = ::volume;
    }
}

struct ImpExRad : RED4ext::IScriptable
{
    RED4ext::CClass* GetNativeType();
};

RED4ext::TTypedClass<ImpExRad> customClass("ImpExRad");
RED4ext::CClass* ImpExRad::GetNativeType()
{
    return &customClass;
}

RED4EXT_C_EXPORT void RED4EXT_CALL RegisterTypes()
{
    RED4ext::CRTTISystem::Get()->RegisterType(&customClass);
}
RED4EXT_C_EXPORT void RED4EXT_CALL PostRegisterTypes()
{
    auto rtti = RED4ext::CRTTISystem::Get();
    auto scriptable = rtti->GetClass("IScriptable");
    customClass.parent = scriptable;

    // Set Current Media Volume (0.0f to 1.0f)
    auto setvolfunc = RED4ext::CClassFunction::Create(&customClass, "SetMediaVolume", "SetMediaVolume", &SetMediaVolume,
                                                      {.isNative = true});
    setvolfunc->AddParam("Float", "newVolume");
    customClass.RegisterFunction(setvolfunc);

    // Set Mode
    auto setmodefunc =
        RED4ext::CClassFunction::Create(&customClass, "SetMode", "SetMode", &SetMode, {.isNative = true});
    setmodefunc->AddParam("Int32", "newMode");
    customClass.RegisterFunction(setmodefunc);

    // Get Mode
    auto getmodefunc =
        RED4ext::CClassFunction::Create(&customClass, "GetMode", "GetMode", &GetMode, {.isNative = true});
    getmodefunc->SetReturnType("Int32");
    customClass.RegisterFunction(getmodefunc);

    // Pause Media
    auto pausefunc =
        RED4ext::CClassFunction::Create(&customClass, "PauseMedia", "PauseMedia", &PauseMedia, {.isNative = true});
    customClass.RegisterFunction(pausefunc);

    // Resume Media
    auto resumefunc =
        RED4ext::CClassFunction::Create(&customClass, "ResumeMedia", "ResumeMedia", &ResumeMedia, {.isNative = true});
    customClass.RegisterFunction(resumefunc);

    // Reload device function
    auto relfunc = RED4ext::CClassFunction::Create(&customClass, "ReloadDevices", "ReloadDevices", &ReloadDevices,
                                                   {.isNative = true});
    customClass.RegisterFunction(relfunc);

    // Devices Getter
    auto devgetfunc = RED4ext::CClassFunction::Create(&customClass, "GetDevicesList", "GetDevicesList", &GetDevicesList,
                                                      {.isNative = true});
    devgetfunc->SetReturnType("array:String");
    customClass.RegisterFunction(devgetfunc);

    // Device Setter
    auto devsetfunc =
        RED4ext::CClassFunction::Create(&customClass, "SetDevice", "SetDevice", &SetDevice, {.isNative = true});
    devsetfunc->AddParam("String", "newDevice");
    customClass.RegisterFunction(devsetfunc);

    // Set Use Device
    auto usedevfunc = RED4ext::CClassFunction::Create(&customClass, "SetUseDevice", "SetUseDevice", &SetUseDevice,
                                                      {.isNative = true});
    usedevfunc->AddParam("Bool", "val");
    customClass.RegisterFunction(usedevfunc);

    // Set Use App
    auto useappfunc =
        RED4ext::CClassFunction::Create(&customClass, "SetUseApp", "SetUseApp", &SetUseApp, {.isNative = true});
    useappfunc->AddParam("Bool", "val");
    customClass.RegisterFunction(useappfunc);

    // Get Index Mode
    auto getappfunc = RED4ext::CClassFunction::Create(&customClass, "GetAppIndex", "GetAppIndex", &GetAppIndex, {.isNative = true});
    getappfunc->SetReturnType("Int32");
    customClass.RegisterFunction(getappfunc);

    // Set Index App
    auto setappfunc =
        RED4ext::CClassFunction::Create(&customClass, "SetAppIndex", "SetAppIndex", &SetAppIndex, {.isNative = true});
    setappfunc->AddParam("Int32", "val");
    customClass.RegisterFunction(setappfunc);

    // Get Volume
    auto getvolfunc =
        RED4ext::CClassFunction::Create(&customClass, "GetVolume", "GetVolume", &GetVolume, {.isNative = true});
    getvolfunc->SetReturnType("Float");
    customClass.RegisterFunction(getvolfunc);
}

RED4EXT_C_EXPORT bool RED4EXT_CALL Main(RED4ext::v1::PluginHandle aHandle, RED4ext::v1::EMainReason aReason,
                                        const RED4ext::v1::Sdk* aSdk)
{
    switch (aReason)
    {
    case RED4ext::v1::EMainReason::Load:
    {
        auto rtti = RED4ext::CRTTISystem::Get();
        g_pluginHandle = aHandle;
        g_sdk = aSdk;

        rtti->AddRegisterCallback(RegisterTypes);
        rtti->AddPostRegisterCallback(PostRegisterTypes);

        // g_sdk->logger->Info(g_pluginHandle, "Loaded successfully!");

        ReloadDevicesInternal();

        if (::devices.size() == 0)
        {
            aSdk->logger->Error(aHandle, "Can't get devices.");
        }

        break;
    }
    case RED4ext::v1::EMainReason::Unload:
    {
        break;
    }
    }

    return true;
}

RED4EXT_C_EXPORT void RED4EXT_CALL Query(RED4ext::v1::PluginInfo* aInfo)
{
    aInfo->name = L"CP2077.Improved.External.Radio";
    aInfo->author = L"unstblr, GALAXIATHE1";
    aInfo->version = RED4EXT_V1_SEMVER(1, 0, 0);
    aInfo->runtime = RED4EXT_V1_RUNTIME_VERSION_LATEST;
    aInfo->sdk = RED4EXT_V1_SDK_VERSION_CURRENT;
}

RED4EXT_C_EXPORT uint32_t RED4EXT_CALL Supports()
{
    return RED4EXT_API_VERSION_1;
}


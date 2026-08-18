#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Containers/DynArray.hpp>
#include <RED4ext/CString.hpp>

// Fetch audio devices on Windows.
#include <windows.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <endpointvolume.h>

// Pause/Play functionality
#pragma comment(lib, "runtimeobject.lib")
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.Control.h>
#include <thread>

static int32_t mode = 0; // 0 = pause when out of car, 1 = mute when out of car, 2 = don't do anything

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


//
// FIX THIS SO IT CAN TARGET OTHER AUDIO DEVICE INSTEAD OF THE MAIN ONE
// FUNCTIONAL JUST WRONG DEVICE
// (IT TARGET THE APPLICATION ON THE SAME DEVICE NOT OTHER)
//
static void SetCurrentSessionVolumeInternal(float volumeLevel)
{
    HRESULT hrCo = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    bool shouldUninit = SUCCEEDED(hrCo);

    try
    {
        winrt::init_apartment(winrt::apartment_type::multi_threaded);

        auto manager = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
        auto currentSession = manager.GetCurrentSession();

        if (!currentSession)
        {
            if (shouldUninit)
                CoUninitialize();
            return;
        }

        // Clamp volume level (0.0 to 1.0)
        float clampedVol = (volumeLevel < 0.0f) ? 0.0f : ((volumeLevel > 1.0f) ? 1.0f : volumeLevel);

        // Enumerate active audio sessions in WASAPI
        IMMDeviceEnumerator* pEnumerator = nullptr;
        if (SUCCEEDED(CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                                       (void**)&pEnumerator)) &&
            pEnumerator)
        {
            IMMDevice* pDevice = nullptr;
            if (SUCCEEDED(pEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &pDevice)) && pDevice)
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

                        // Loop through sessions and apply volume to non-system active media sessions
                        for (int i = 0; i < count; i++)
                        {
                            IAudioSessionControl* pControl = nullptr;
                            if (SUCCEEDED(pSessionEnumerator->GetSession(i, &pControl)) && pControl)
                            {
                                ISimpleAudioVolume* pVolume = nullptr;
                                if (SUCCEEDED(
                                        pControl->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&pVolume)) &&
                                    pVolume)
                                {
                                    pVolume->SetMasterVolume(clampedVol, NULL);
                                    pVolume->Release();
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
    catch (...)
    {
    }

    if (shouldUninit)
    {
        CoUninitialize();
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

    if (g_sdk && g_pluginHandle)
    {
        g_sdk->logger->InfoF(g_pluginHandle, "Setting current media session volume to: %.2f", newVolume);
    }

    SetCurrentMediaVolumeAsync(newVolume);
}

void SetMode(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, void* aOut, int64_t a4)
{
    int newMode;
    RED4ext::GetParameter(aFrame, &newMode);
    aFrame->code++;

    mode = newMode;
}
void GetMode(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, int32_t* aOut, int64_t a4) 
{
    aFrame->code++;
    if (aOut)
    {
        *aOut = ::mode;
    }
}

void PauseMedia(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, void* aOut, int64_t a4)
{
    // pause current media using
    aFrame->code++;
    PauseMediaAsync();
}
void ResumeMedia(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, void* aOut, int64_t a4)
{
    // resume current media
    aFrame->code++;
    ResumeMediaAsync();
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

        g_sdk->logger->Info(g_pluginHandle, "Loaded successfully!");

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


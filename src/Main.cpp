#include <RED4ext/RED4ext.hpp>

// Fetch audio devices on Windows.
#include <iostream>
#include <windows.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>
#include <vector>
#include <string>

static std::vector<std::string> devices = {};
static std::vector<std::string> guids = {};

static std::string currGuid = "";
static BOOL isActive = false;

void GetDevicesList(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, std::vector<std::string>* aOut, int64_t a4)
{
    aFrame->code++;

    if (aOut)
    {
        *aOut = ::devices;
    }
}

std::vector<std::string> ReloadDevice()
{
    std::vector<std::string> devicesList = {};

    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr))
        return devicesList;

    IMMDeviceEnumerator* pEnumerator = NULL;
    IMMDeviceCollection* pCollection = NULL;

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                          (void**)&pEnumerator);

    if (SUCCEEDED(hr))
    {
        // Enumerate active audio output (render) devices
        hr = pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pCollection);
    }

    if (SUCCEEDED(hr))
    {
        UINT count;
        pCollection->GetCount(&count);

        for (UINT i = 0; i < count; i++)
        {
            IMMDevice* pEndpoint = NULL;
            hr = pCollection->Item(i, &pEndpoint);

            if (SUCCEEDED(hr))
            {
                IPropertyStore* pProps = NULL;
                hr = pEndpoint->OpenPropertyStore(STGM_READ, &pProps);

                if (SUCCEEDED(hr))
                {
                    PROPVARIANT varName;
                    PropVariantInit(&varName);

                    // Get the friendly name of the audio device
                    hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
                    if (SUCCEEDED(hr))
                    {
                        // log all devices
                        std::wstring wname(varName.pwszVal);
                        std::string narrowName(wname.begin(), wname.end());

                        // std::string name = std::format("Device {0}: {1}", i, narrowName);
                        devicesList.push_back(narrowName);
                    }

                    PropVariantClear(&varName);
                    pProps->Release();
                }
                pEndpoint->Release();
            }
        }
        pCollection->Release();
    }

    if (pEnumerator)
        pEnumerator->Release();
    CoUninitialize();
    return devicesList;
}

std::string WideToUTF8(const wchar_t* wstr)
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

std::vector<std::string> ReloadDeviceGUID()
{
    std::vector<std::string> devicesList;

    // Initialize COM cleanly for multithreaded/DLL execution
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
                    LPWSTR pwszID = nullptr;
                    // GetId returns the WASAPI Endpoint ID string
                    if (SUCCEEDED(pEndpoint->GetId(&pwszID)) && pwszID)
                    {
                        devicesList.push_back(WideToUTF8(pwszID));
                        CoTaskMemFree(pwszID); // Free memory allocated by GetId
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

    return devicesList;
}

void ReloadDevices(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, void* aOut, int64_t a4)
{
    ::devices = ReloadDevice();
    ::guids = ReloadDeviceGUID();
}

void SetDeviceVolume(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, void* aOut, int64_t a4)
{
    float newVolume;
    RED4ext::GetParameter(aFrame, &newVolume);
    aFrame->code++;

    // Set target device volume (currGuid) and change it volume to match ingame radio.
}

void SetDevice(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, void* aOut, int64_t a4)
{
    std::string newDevice;
    RED4ext::GetParameter(aFrame, &newDevice);
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

    // Set device volume function
    auto volfunc = RED4ext::CClassFunction::Create(&customClass, "SetDeviceVolume", "SetDeviceVolume", &SetDeviceVolume,
                                                {.isNative = true});
    volfunc->AddParam("Float", "newVolume");
    customClass.RegisterFunction(volfunc);

    // Reload device function
    auto relfunc = RED4ext::CClassFunction::Create(&customClass, "ReloadDevice", "ReloadDevice", &ReloadDevices,
                                                {.isNative = true});
    customClass.RegisterFunction(relfunc);

    // Devices Getter
    auto devgetfunc = RED4ext::CClassFunction::Create(&customClass, "GetDevicesList", "GetDevicesList", &GetDevicesList,
                                                      {.isNative = true});
    devgetfunc->SetReturnType("array:String");
    customClass.RegisterFunction(devgetfunc);

    // Device Setter
    auto devsetfunc =
        RED4ext::CClassFunction::Create(&customClass, "SetDevice", "SetDevice", &SetDevice,
                                                      {.isNative = true});
    devsetfunc->AddParam("String", "newDevice");
    customClass.RegisterFunction(devsetfunc);
}

RED4EXT_C_EXPORT bool RED4EXT_CALL Main(RED4ext::v1::PluginHandle aHandle, RED4ext::v1::EMainReason aReason,
                                        const RED4ext::v1::Sdk* aSdk)
{
    switch (aReason)
    {
    case RED4ext::v1::EMainReason::Load:
    {
        auto rtti = RED4ext::CRTTISystem::Get();

        rtti->AddRegisterCallback(RegisterTypes);
        rtti->AddPostRegisterCallback(PostRegisterTypes);

        // aSdk->logger->Info(aHandle, "List of devices gathered.");

        ::devices = ReloadDevice();

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


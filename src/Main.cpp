#include <RED4ext/RED4ext.hpp>

// Fetch audio devices on Windows.
#include <iostream>
#include <windows.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>

int GetAllDevices(RED4ext::v1::PluginHandle aHandle, const RED4ext::v1::Sdk* aSdk)
{
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr))
        return 1;

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

                        std::string name = std::format("Device {0}: {1}", i, narrowName);
                        aSdk->logger->Info(aHandle, name.c_str());
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
    return 0;
}



RED4EXT_C_EXPORT bool RED4EXT_CALL Main(RED4ext::v1::PluginHandle aHandle, RED4ext::v1::EMainReason aReason,
                                        const RED4ext::v1::Sdk* aSdk)
{
    switch (aReason)
    {
    case RED4ext::v1::EMainReason::Load:
    {
        /*
         * Here you can register your custom functions, initalize variable, create hooks and so on.
         *
         * Be sure to store the plugin handle and the interface because you cannot get it again later. The plugin handle
         * is what identify your plugin through the extender.
         *
         * Returning "true" in this function loads the plugin, returning "false" will unload it and "Main" will be
         * called with "Unload" reason.
         */

        if (GetAllDevices(aHandle,aSdk) != 0)
        {
            // log errors
            aSdk->logger->Error(aHandle, "Can't Initialize.");
        }

        aSdk->logger->Info(aHandle, "List of devices gathered.");


        break;
    }
    case RED4ext::v1::EMainReason::Unload:
    {
        /*
         * Here you can free resources you allocated during initalization or during the time your plugin was executed.
         */
        break;
    }
    }

    /*
     * For more information about this function see https://docs.red4ext.com/mod-developers/creating-a-plugin#main.
     */

    return true;
}

RED4EXT_C_EXPORT void RED4EXT_CALL Query(RED4ext::v1::PluginInfo* aInfo)
{
    /*
     * This function supply the necessary information about your plugin, like name, version, support runtime and SDK. DO
     * NOT do anything here yet!
     *
     * You MUST have this function!
     *
     * Make sure to fill all of the fields here in order to load your plugin correctly.
     *
     * Runtime version is the game's version, it is best to let it set to "RED4EXT_RUNTIME_LATEST" if you want to target
     * the latest game's version that the SDK defined, if the runtime version specified here and the game's version do
     * not match, your plugin will not be loaded. If you want to use RED4ext only as a loader and you do not care about
     * game's version use "RED4EXT_RUNTIME_INDEPENDENT".
     *
     * For more information about this function see https://docs.red4ext.com/mod-developers/creating-a-plugin#query.
     */

    aInfo->name = L"CP2077.Improved.External.Radio";
    aInfo->author = L"unstblr, GALAXIATHE1";
    aInfo->version = RED4EXT_V1_SEMVER(1, 0, 0);
    aInfo->runtime = RED4EXT_V1_RUNTIME_VERSION_LATEST;
    aInfo->sdk = RED4EXT_V1_SDK_VERSION_CURRENT;
}

RED4EXT_C_EXPORT uint32_t RED4EXT_CALL Supports()
{
    /*
     * This functions returns only what API version is support by your plugins.
     * You MUST have this function!
     *
     * For more information about this function see https://docs.red4ext.com/mod-developers/creating-a-plugin#supports.
     */
    return RED4EXT_API_VERSION_1;
}


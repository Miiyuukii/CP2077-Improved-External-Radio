# Cyberpunk 2077 Improved External Radio
### based on [DrJackieBright's CP77-External-Radio-red4ext](https://github.com/DrJackieBright/CP77-External-Radio-red4ext/)

This mod is basically DrJackieBright's mod with more features.

## How this work
This mod can control current media that's playing and control its volume via "Window Volume Mixer" in other devices or current one.

![volume mixer](images/volMixerUncut.png)

![media pause/play](images/mediaCut.png)

# Features

## Volume Control
You can control the volume of specific devices and/or application with in-game radio volume.

![native ui setting_menu_device](images/modSettingDevCut.png)

![in-game radio menu](images/radioCut.png)

## Pause/Mute Behavior
You can choose whether it behaves.
- Pause on unmount from vehicles.
- Mute on unmount from vehicles.
- Do nothing

![native ui setting menu_pause_mute](images/modSettingBehCut.png)

## Supported Applications
- Spotify
- Google Chrome
- Firefox
- Microsoft Edge
- VLC media player
- foobar2000
- Apple Music
- Tidal
- MusicBee
- Windows Media Player
- AIMP
- Opera
- Brave
- Discord

![native ui setting menu_app](images/modSettingAppCut.png)

# Requirements
Only supported Windows (10/11) [I'm not sure about Windows 11 but, should be working mostly]

Latest version of Cyberpunk 2077 (DLC not required)


## $\color{#ff0000}\textsf{Caution}$
$\color{#ff0000}{\text{When option "Use specific app" is not on.}}$

$\color{#ff0000}{\text{It'll control all of the volume in the device you chose or the current one if "Use specific device" is not on.}}$

$\color{#ff0000}{\text{This is for people that have customized their volume control in "Window Volume Mixer"}}$

$\color{#ff0000}{\text{Be careful!}}$


# Installation
1. Install [redscript](https://www.nexusmods.com/cyberpunk2077/mods/1511), [RED4ext](https://www.nexusmods.com/cyberpunk2077/mods/2380), [Cyber Engine Tweaks](https://www.nexusmods.com/cyberpunk2077/mods/107), [Native Setting UI](https://www.nexusmods.com/cyberpunk2077/mods/3518), [TweakXL](https://www.nexusmods.com/cyberpunk2077/mods/4197).
2. Unzip the file into your game directory or use Vortex to install.
3. Configure it in "Mods/CP2077 Improved External Radio".

# Troubleshoot
Make sure to update Visual C++ Redistributable on your system. [Link here](https://aka.ms/vs/17/release/vc_redist.x64.exe)

# Recommendation
This is my personal recommendation. 

A.

  - Installing Voicemeeter and install their VB-Cable
  
  - After you set up your voicemeeter, Point your application audio to VB-Cable (Cable Input)
  
  - Then target that device by toggling "Use specific device" and choose "Cable Input" in mod settings.
  
  - (This method ensures that even if your application isn't supported by the mod. You can still use this mod to control its volume.)
  
B.

  - Specify the application that you're going to control by toggling "Use specific app" and select your app below.
  
C.

  - Combine both cuz why not :)

# Compatibility
Not compatible with [DrJackieBright's CP77-External-Radio-red4ext](https://github.com/DrJackieBright/CP77-External-Radio-red4ext/) as I'm using her configuration.

# Big thanks to
[DrJackieBright](https://github.com/DrJackieBright/CP77-External-Radio-red4ext/)'s CP77-External-Radio-red4ext for being an inspiration for me to create this mod in the first place.

[justarandomguyintheinternet](https://github.com/justarandomguyintheinternet/)'s Native UI Settings

[j4cekm4](https://github.com/jac3km4/)'s redscript

[wopss](https://github.com/wopss/)'s RED4ext

[maximegmd](https://github.com/maximegmd/)'s Cyber Engine Tweak

# License
See [License](https://github.com/Miiyuukii/CP2077.Improved.External.Radio/blob/master/LICENSE.md) tab for more information.

# AI Notices
I've used AI to help with coding, debugging process. Mostly on C++ side of things.

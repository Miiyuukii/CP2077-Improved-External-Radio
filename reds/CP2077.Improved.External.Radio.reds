
public native class ImpExRad extends IScriptable {
  public native func SetMediaVolume(newVolume: Float) -> Void;

  public native func GetMode() -> Int32;

  public native func SetMode(newMode: Int32) -> Void;

  public native func PauseMedia() -> Void;

  public native func ResumeMedia() -> Void;
}

@wrapMethod(RadioVolumeSettingsController)
private func ChangeValue(forward: Bool) -> Void {
  wrappedMethod(forward);

  let congiVar: ref<ConfigVarInt> = this.m_SettingsEntry as ConfigVarInt;
  let targetVolume: Float = 1.0;

  if IsDefined(congiVar) {
    targetVolume = Cast<Float>(congiVar.GetValue()) / 100.0;
  }

  let bridge: ref<ImpExRad> = new ImpExRad();
  bridge.SetMediaVolume(targetVolume);
}

@wrapMethod(PlayerPuppet)
private cb func OnMountingEvent(evt: ref<MountingEvent>) -> Bool {
  let ret = wrappedMethod(evt);

  if this.GetPS().isEnabled && Equals(evt.request.lowLevelMountingInfo.slotId.id, n"seat_front_left") {
    let vehicleObject: ref<VehicleObject> = GameInstance.FindEntityByID(this.GetGame(), evt.request.lowLevelMountingInfo.parentId) as VehicleObject;
    vehicleObject.ToggleRadioReceiver(false);

    let bridge: ref<ImpExRad> = new ImpExRad();
    let currentMode: Int32 = bridge.GetMode();

    let settingsSystem: ref<UserSettings> = GameInstance.GetSettingsSystem(this.GetGame());
    let radioVolumeVar: ref<ConfigVarFloat> = settingsSystem.GetVar(n"/audio/volume", n"Radio") as ConfigVarFloat;
    let targetVolume: Float = 1.0;

    if IsDefined(radioVolumeVar) {
      targetVolume = radioVolumeVar.GetValue() / 100.0;
    }

    switch currentMode {
      case 0:
        // Resume / Unmute on enter
        bridge.SetMediaVolume(targetVolume);
        bridge.ResumeMedia();
        break;
      case 1:
        // Restore device volume on enter
        bridge.SetMediaVolume(targetVolume);
        break;
      case 2:
        // Don't do anything
        break;
      default:
        break;
    }
  }

  return ret;
}

@wrapMethod(PlayerPuppet)
private cb func OnUnmountingEvent(evt: ref<UnmountingEvent>) -> Bool {
  let ret = wrappedMethod(evt);

  if this.GetPS().isEnabled && Equals(evt.request.lowLevelMountingInfo.slotId.id, n"seat_front_left") {
    let bridge: ref<ImpExRad> = new ImpExRad();
    let currentMode: Int32 = bridge.GetMode();

    switch currentMode {
      case 0:
        // Pause / Mute on exit
        bridge.SetMediaVolume(0.0);
        bridge.PauseMedia();
        break;
      case 1:
        // Mute on exit
        bridge.SetMediaVolume(0.0);
        break;
      case 2:
        // Don't do anything
        break;
      default:
        break;
    }
  }

  return ret;
}

@addField(PlayerPuppetPS)
public persistent let isEnabled: Bool;

@wrapMethod(VehicleRadioPopupGameController)
protected func Activate() -> Void {
  if !IsDefined(this.m_selectedItem) {
    return;
  }
  let bridge: ref<ImpExRad> = new ImpExRad();
  let player: wref<PlayerPuppet> = GetPlayer(this.m_playerPuppet.GetGame());

  if this.m_selectedItem.GetStationData().m_record.Index() == -2 {
    this.m_quickSlotsManager.SendRadioEvent(false, false, -1);
    player.GetPS().isEnabled = true;
    bridge.ResumeMedia();
  } else {
    if player.GetPS().isEnabled {
      player.GetPS().isEnabled = false;
      bridge.PauseMedia();
    }
    wrappedMethod();
  }
}

@wrapMethod(VehicleComponent)
protected cb func OnVehicleRadioEvent(evt: ref<VehicleRadioEvent>) -> Bool {
  let ret = wrappedMethod(evt);
  if this.m_radioState && GetPlayer(this.GetVehicle().GetGame()).GetPS().isEnabled {
    GetPlayer(this.GetVehicle().GetGame()).GetPS().isEnabled = false;
    let bridge: ref<ImpExRad> = new ImpExRad();
    bridge.PauseMedia();
  }
  return ret;
}

@wrapMethod(VehiclesManagerDataHelper)
public final static func GetRadioStations(player: ref<GameObject>) -> array<ref<IScriptable>> {
  let res = wrappedMethod(player);
  let auxArray: array<ref<IScriptable>>;
  VehiclesManagerDataHelper
    .PushRadioStationData(auxArray, TweakDBInterface.GetRadioStationRecord(t"RadioStation.Aux"));
  ArrayInsert(res, 1, auxArray[0]);
  return res;
}


# gpd-charge-limit

A small Linux kernel module that exposes the GPD Win Mini 2025's battery charge limit, so you can change it from the OS
instead of rebooting into the BIOS menu.

GNOME's "Preserve Battery Health" toggle, KDE's
battery settings and `upower` should drive it automatically.

## Requirements

- GPD Win Mini 2025 (AMD AI 9 HX370) (`G1617-02`)
- [v2.10 BIOS Firmware & v2.07 EC Firmware](https://www.gpd.hk/gpdwinmini2025firmwaredriver) (with charge limiting feature) installed.
- Linux kernel 6.14 or newer.

**Note:** I don't own the 8840U variant of this device so I cannot test if it works.

## Installation

### DKMS

```sh
git clone https://github.com/cvcore/gpd-charge-limit
sudo cp -r gpd-charge-limit /usr/src/gpd-charge-limit-0.1.0
sudo dkms add     -m gpd-charge-limit -v 0.1.0
sudo dkms install -m gpd-charge-limit -v 0.1.0
```

### NixOS (flake)

```nix
{
  inputs.gpd-charge-limit.url = "github:cvcore/gpd-charge-limit";

  # in your nixosSystem modules:
  imports = [ inputs.gpd-charge-limit.nixosModules.default ];
  hardware.gpd-charge-limit.enable = true;
}
```

### Manual build

```sh
make
sudo insmod gpd_charge_limit.ko
```

## Usage

```sh
# read the current limit
cat /sys/class/power_supply/BATT/charge_control_end_threshold

# stop charging at 85%
echo 85 | sudo tee /sys/class/power_supply/BATT/charge_control_end_threshold
# Accepted values 60 - 100
```

## Risks

- **Only 60–100 is accepted.** At power-on the EC treats values below 60 as 100, so a lower
  setting would silently mean "no limit". The module rejects them rather than let that happen.
- **Avoid writing the limit frequently.** The EC appears to persist changed values to its own
  flash. Setting it occasionally is fine; polling it in a loop is not.
- **The BIOS rewrites `0x1D` on every boot**, so the BIOS menu value is the boot default and
  anything set through sysfs lasts until reboot. Set it in the BIOS too if you want it to stick.
- Charging stops at the limit and resumes at `limit - 10` (95 when the limit is ≥ 96). That
  hysteresis is EC behaviour, not something this module controls.


## Acknowledgements

Thanks to the authors of the mainline [`gpd-fan-driver`](https://github.com/Cryolitia/gpd-fan-driver) for prior art on this hardware family.

GPD is a trademark of its respective owner; this project is not affiliated with or endorsed by
GPD.

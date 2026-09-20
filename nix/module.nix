# NixOS module. Import the flake's nixosModules.default and set
#   hardware.gpd-charge-limit.enable = true;
{
  config,
  lib,
  ...
}:

let
  cfg = config.hardware.gpd-charge-limit;
in
{
  options.hardware.gpd-charge-limit = {
    enable = lib.mkEnableOption ''
      the GPD Win Mini 2025 (G1617-02) battery charge limit driver
    '';
  };

  config = lib.mkIf cfg.enable {
    boot.extraModulePackages = [
      (config.boot.kernelPackages.callPackage ./package.nix { })
    ];

    # The module also autoloads from its DMI alias; listing it here makes the
    # dependency explicit and loads it early.
    boot.kernelModules = [ "gpd_charge_limit" ];
  };
}

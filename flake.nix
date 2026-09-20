{
  description = "GPD Win Mini 2025 (G1617-02) battery charge limit driver";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs =
    { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = nixpkgs.legacyPackages.${system};

      # Kernel sets the CI matrix builds against. Deliberately symbolic rather
      # than pinned versions (linux_6_14 etc.): nixpkgs deletes kernels once
      # they go EOL upstream, so a hardcoded list turns into eval errors. These
      # two names always exist and span the supported range.
      #
      # The driver's floor is 6.14 -- where power_supply_register_extension()
      # and struct power_supply_ext appeared -- enforced by meta.broken in
      # nix/package.nix rather than by a build target, since nixpkgs no longer
      # ships a 6.14 to build against.
    in
    {
      packages.${system} = {
        gpd-charge-limit = pkgs.linuxPackages.callPackage ./nix/package.nix { };
        gpd-charge-limit-latest = pkgs.linuxPackages_latest.callPackage ./nix/package.nix { };
        default = self.packages.${system}.gpd-charge-limit;
      };

      nixosModules.default = ./nix/module.nix;

      # Makes boot.kernelPackages.gpd-charge-limit resolve for consumers who
      # prefer an overlay over the NixOS module.
      overlays.default = final: prev: {
        linuxPackagesFor =
          kernel:
          (prev.linuxPackagesFor kernel).extend (
            lpFinal: lpPrev: {
              gpd-charge-limit = lpFinal.callPackage ./nix/package.nix { };
            }
          );
      };
    };
}

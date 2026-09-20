# Kernel module derivation. Built against whichever kernel the calling
# linuxPackages set provides, e.g.
#   config.boot.kernelPackages.callPackage ./nix/package.nix { }
{
  lib,
  stdenv,
  kernel,
  kernelModuleMakeFlags,
}:

stdenv.mkDerivation {
  pname = "gpd-charge-limit";
  version = "0.1.0";

  # Only the files the build actually reads, so editing the README or CI
  # config does not force a module rebuild.
  src = lib.fileset.toSource {
    root = ../.;
    fileset = lib.fileset.unions [
      ../gpd_charge_limit.c
      ../Makefile
    ];
  };

  nativeBuildInputs = kernel.moduleBuildDependencies;

  makeFlags = kernelModuleMakeFlags ++ [
    "KERNELDIR=${kernel.dev}/lib/modules/${kernel.modDirVersion}/build"
  ];

  installFlags = [
    "DESTDIR=${placeholder "out"}"
    "INSTALL_MOD_DIR=/lib/modules/${kernel.modDirVersion}/extra"
  ];

  meta = {
    description = "Battery charge limit driver for the GPD Win Mini 2025 (G1617-02)";
    homepage = "https://github.com/cvcore/gpd-charge-limit";
    license = lib.licenses.gpl2Only;
    maintainers = [{
      name = "Chengxin Wang";
      github = "cvcore";
    }];
    platforms = [ "x86_64-linux" ];
    # power_supply_register_extension() and struct power_supply_ext appeared
    # in 6.14; absent in 6.13.
    broken = kernel.kernelOlder "6.14";
  };
}

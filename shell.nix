#(import <nixpkgs> {}).pkgsCross.aarch64-multiplatform.callPackage (
#{ mkShell
#, bison
#, flex
#, buildPackages
#, python3
#, ubootPinebook
#}:
#mkShell {
#  nativeBuildInputs = ubootPinebook.nativeBuildInputs;
#  depsBuildBuild = ubootPinebook.depsBuildBuild;
#  hardeningDisable = [ "all" ];
#  #nativeBuildInputs = [
#  #  bison
#  #  flex
#  #  buildPackages.stdenv.cc
#  #  python3
#  #];
#}
#
#) {}
#(import ./. {}).radxa-rock5b.overrideAttrs(_: { src = /var/empty; })
#(import ./. {}).generic-rv1126.overrideAttrs(_: { src = /var/empty; })
(import ./. {}).generic-rk3568.overrideAttrs(_: { src = /var/empty; })

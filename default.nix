{ pkgs ? (import <nixpkgs> {}) }:

let
  escape = builtins.toJSON;
  platFor =
    attr:
    let
      pkgsPlat = builtins.head pkgs.${attr}.meta.platforms;
    in
    plats.${
      pkgsPlat
    } or "Platform ${escape pkgsPlat} not configured for use with pkgsCross."
  ;
  plats = {
    "aarch64-linux" = pkgs.pkgsCross.aarch64-multiplatform;
  };
  u-boot-for =
    attr:
    (platFor attr).${attr}
    .overrideAttrs (oldAttrs: {
      src = builtins.fetchGit ./.;
      postInstall = (oldAttrs.postInstall or "") + ''
        cp -v .config $out/config
      '';
    })
  ;
in
  {
    radxa-rock5b = 
    (u-boot-for "ubootRock5ModelB").overrideAttrs (oldAttrs: {
      version = "master@2025-04-18";
      postInstall = (oldAttrs.postInstall or "") + ''
        FILES=(
          "spl/u-boot-spl.bin"
          "u-boot-rockchip-maskrom.bin"
        )
        cp -v -t $out/ "''${FILES[@]}"
      '';
    })
    ;
  }

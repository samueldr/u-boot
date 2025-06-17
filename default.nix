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
      src =
        # Strip this file from the source.
        # We want this file in the repository, sadly this doesn't work well with builtins.fetchGit.
        builtins.path {
          name = "source";
          path = (builtins.fetchGit ./.);
          filter = path: type: baseNameOf path != "default.nix";
        }
      ;
      postInstall = (oldAttrs.postInstall or "") + ''
        cp -v .config $out/config
      '';
    })
  ;
in
  rec {
    rockchiprs =
      pkgs.callPackage (

        { rustPlatform
        , fetchFromGitHub
        }:
        rustPlatform.buildRustPackage {
          pname = "rockchiprs";
          version = "0-unstable-2025-04-19";
          src = fetchFromGitHub {
            owner = "samueldr";
            repo = "rockchiprs";
            rev = "c07ee79b6597a40f80e71da965a1731be0149a70";
            hash = "sha256-MLBD8FzX+WaaxBj1eWxD+H+OGgiSFuE5I979OX87TYE=";
          };
          cargoHash = "sha256-85hBJPUwDORBrbxr9i11MHa3wGsSIgjl83hANl1etGk";
          buildFeatures = [
            "libusb"
          ];
        }
      ) {}
    ;

    radxa-rock5b =
    (u-boot-for "ubootRock5ModelB").overrideAttrs (oldAttrs: {
      version = "master@2025-04-18";
      postInstall = (oldAttrs.postInstall or "") + ''
        FILES=(
          ".config"
          "spl/u-boot-spl.bin"
          "u-boot-rockchip-maskrom.bin"
        )
        cp -v -t $out/ "''${FILES[@]}"
      '';
    })
    ;

    #
    # Can be used with either of rkdeveloptool or rockusb:
    #
    # $ rkdeveloptool db .../result/rk3588_combined_loader.bin
    # $ cargo run --features="libusb" --example rockusb download-boot .../result/rk3588_combined_loader.bin
    #
    radxa-rock5b-maskrom-uploadable =
      pkgs.callPackage (
        { runCommand
        , rockchiprs
        , rkbin
        , u-boot
        }:
        runCommand "radxa-rock5b-maskrom-uploadable" {
          src = rkbin.src;
          ini = ''
            [CHIP_NAME]
            NAME=RK3588
            [VERSION]
            MAJOR=1
            MINOR=11

            # Data sent to SRAM, to initialize SDRAM and continue in CODE472
            [CODE471_OPTION]
            NUM=1
            Path1=${rkbin.TPL_RK3588}
            Sleep=1

            # Data sent to SDRAM
            [CODE472_OPTION]
            NUM=1
            Path1=${u-boot}/u-boot-rockchip-maskrom.bin

            # This section needs to exist.
            [LOADER_OPTION]
            NUM=1
            LOADER1=null
            null=/dev/null

            [OUTPUT]
            PATH=rk3588_combined_loader.bin

            #
            # RK3588 settings
            #
            [SYSTEM]
            NEWIDB=true
            # Disables(?) RC4 on DDR and Loader entries(?)
            [FLAG]
            471_RC4_OFF=true
            RC4_OFF=true
          '';
          passAsFile = [ "ini" ];
        } ''
        cp --recursive --target-directory . "$src"/{tools,bin,RKBOOT}
        chmod --recursive +w *

        (PS4=" $ "; set -x
        cat "$iniPath" > config.ini
        bin="$(grep '^PATH=' "config.ini" | cut -d'=' -f2)"

        # Merge the binary
        tools/boot_merger "config.ini"

        # Strip the timestamp
        # boot_merger is a static binary, libfaketime won't work.
        dd if=/dev/zero of="$bin" bs=1 seek="$((0x0E))" count=7 conv=notrunc
        # Fix the CRC
        "${rockchiprs}/bin/rockfile" update-crc "$bin"

        # Install
        mkdir -p "$out"
        mv -t "$out" *.bin
        )
        ''
      ) {
        inherit rockchiprs;
        u-boot = radxa-rock5b;
      }
    ;
  }

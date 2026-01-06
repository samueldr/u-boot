{ pkgs ? (import pkgsPath {})
, pkgsPath ?
  builtins.fetchTarball {
    url = "https://github.com/NixOS/nixpkgs/archive/ee930f9755f58096ac6e8ca94a1887e0534e2d81.tar.gz";
    sha256 = "sha256:15zn3jbphw6fwv0x7w9i4nqz3yvi8zpjy21d5jd5nvcpsvi4l7ra";
  }
}:

let
  this_src = 
    # Strip this file from the source.
    # We want this file in the repository, sadly this doesn't work well with builtins.fetchGit.
    builtins.path {
      name = "source";
      path = (builtins.fetchGit ./.);
      filter = path: type: baseNameOf path != "default.nix";
    }
  ;

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
    "armv7l-linux" = pkgs.pkgsCross.armv7l-hf-multiplatform;
  };
  u-boot-for =
    attrOrDrv:
    let
      drv =
        if builtins.isString attrOrDrv then
          (platFor attrOrDrv).${attrOrDrv}
        else
          attrOrDrv
      ;
    in
    drv.overrideAttrs (oldAttrs: {
      version = "master@2025-04-18";
      src = this_src;
      postInstall = (oldAttrs.postInstall or "") + ''
        cp -v .config $out/config
      '';
    })
  ;

  make-64bit-rockchip-u-boot =
    attr: cfg:
    (u-boot-for attr).overrideAttrs (oldAttrs: {
      extraConfig = ''
        CONFIG_SPL_RAM_DEVICE=y
        CONFIG_SPL_RAM_SUPPORT=y
      '';
      postInstall = (oldAttrs.postInstall or "") + ''
        FILES=(
          "spl/u-boot-spl.bin"
          "u-boot-rockchip-maskrom.bin"
        )
        cp -v -t $out/ "''${FILES[@]}"
      '';
    } // cfg)
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

    #
    # Can be used with either of rkdeveloptool or rockusb:
    #
    # $ rkdeveloptool db .../result/rk3588_combined_loader.bin
    # $ cargo run --features="libusb" --example rockusb download-boot .../result/rk3588_combined_loader.bin
    #
    # FIXME: add per-SoC knowledge.
    make-maskrom-uploadable =
      { u-boot }:
      pkgs.callPackage (
        { runCommand
        , rockchiprs
        , rkbin
        , u-boot
        }:
        runCommand "${u-boot.defconfig}-maskrom-uploadable" {
          src = rkbin.src;
          # 3568
          ini = ''
            [CHIP_NAME]
            NAME=RK3568
            [VERSION]
            MAJOR=1
            MINOR=11

            # Data sent to SRAM, to initialize SDRAM and continue in CODE472
            [CODE471_OPTION]
            NUM=1
            Path1=${rkbin.TPL_RK3568}
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
            PATH=rk3568_combined_loader.bin

            #
            # RK3568 settings
            #
            [SYSTEM]
            NEWIDB=true
            # Disables(?) RC4 on DDR and Loader entries(?)
            [FLAG]
            471_RC4_OFF=true
            RC4_OFF=true
          '';
          ## 3588
          #ini = ''
          #  [CHIP_NAME]
          #  NAME=RK3588
          #  [VERSION]
          #  MAJOR=1
          #  MINOR=11

          #  # Data sent to SRAM, to initialize SDRAM and continue in CODE472
          #  [CODE471_OPTION]
          #  NUM=1
          #  Path1=${rkbin.TPL_RK3588}
          #  Sleep=1

          #  # Data sent to SDRAM
          #  [CODE472_OPTION]
          #  NUM=1
          #  Path1=${u-boot}/u-boot-rockchip-maskrom.bin

          #  # This section needs to exist.
          #  [LOADER_OPTION]
          #  NUM=1
          #  LOADER1=null
          #  null=/dev/null

          #  [OUTPUT]
          #  PATH=rk3588_combined_loader.bin

          #  #
          #  # RK3588 settings
          #  #
          #  [SYSTEM]
          #  NEWIDB=true
          #  # Disables(?) RC4 on DDR and Loader entries(?)
          #  [FLAG]
          #  471_RC4_OFF=true
          #  RC4_OFF=true
          #'';
          ##ini = ''
          ##  [CHIP_NAME]
          ##  NAME=RV1126
          ##  [VERSION]
          ##  MAJOR=1
          ##  MINOR=5
          ##  [CODE471_OPTION]
          ##  NUM=1
          ##  Path1=${/Users/samuel/SBCs/rockchip/rkbin/bin/rv11/rv1126_ddr_924MHz_v1.14.bin}
          ##  Sleep=1
          ##  [CODE472_OPTION]
          ##  NUM=1
          ##  Path1=${u-boot}/u-boot-rockchip-maskrom.bin

          ##  ### [CHIP_NAME]
          ##  ### NAME=RV1126
          ##  ### [VERSION]
          ##  ### MAJOR=1
          ##  ### MINOR=5
          ##  ### #[CHIP_NAME]
          ##  ### #NAME=RK3588
          ##  ### #[VERSION]
          ##  ### #MAJOR=1
          ##  ### #MINOR=11

          ##  ### # Data sent to SRAM, to initialize SDRAM and continue in CODE472
          ##  ### [CODE471_OPTION]
          ##  ### NUM=1
          ##  ### #Path1=${rkbin.TPL_RK3588}
          ##  ### #Path1=${u-boot}/u-boot-tpl.bin
          ##  ### Path1=${/Users/samuel/SBCs/rockchip/rkbin/bin/rv11/rv1126_ddr_924MHz_v1.14.bin}
          ##  ### #
          ##  ### Sleep=1

          ##  ### # Data sent to SDRAM
          ##  ### [CODE472_OPTION]
          ##  ### NUM=1
          ##  ### Path1=${u-boot}/u-boot-rockchip-maskrom.bin

          ##  # This section needs to exist.
          ##  [LOADER_OPTION]
          ##  NUM=1
          ##  LOADER1=null
          ##  null=/dev/null

          ##  [OUTPUT]
          ##  PATH=rv1126_combined_loader.bin

          ##  ### # #
          ##  ### # # RK3588 settings
          ##  ### # #
          ##  ### # [SYSTEM]
          ##  ### # NEWIDB=true
          ##  ### # # Disables(?) RC4 on DDR and Loader entries(?)
          ##  ### # [FLAG]
          ##  ### # 471_RC4_OFF=true
          ##  ### # RC4_OFF=true
          ##'';
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
        inherit rockchiprs u-boot;
      }
    ;

    radxa-rock5b = make-64bit-rockchip-u-boot "ubootRock5ModelB" {};
    radxa-rock5b-maskrom-uploadable = make-maskrom-uploadable {
      u-boot = radxa-rock5b;
    };

    # NOTE: still borrow's Nixpkgs' ubootRock5ModelB as a starting point.
    generic-rk3568 = ((make-64bit-rockchip-u-boot "ubootRock5ModelB" {}).override {
      defconfig = "generic-rk3568_defconfig";
      filesToInstall = [
        "u-boot.itb"
        "idbloader.img"
        "u-boot-rockchip.bin"
      ];
    }).overrideAttrs(_: {
      ROCKCHIP_TPL = pkgs.rkbin.TPL_RK3566;
    });
    #  $ $(nix-build --no-out-link --attr rockchiprs)/bin/rockusb download-boot $(nix-build --no-out-link --attr generic-rk3568-maskrom-uploadable)/*.bin
    # ```
    generic-rk3568-maskrom-uploadable = make-maskrom-uploadable {
      u-boot = generic-rk3568;
    };

    # NOTE: still borrow's Nixpkgs' ubootRock5ModelB as a starting point.
    generic-rk3588 = (make-64bit-rockchip-u-boot "ubootRock5ModelB" {}).override {
      defconfig = "generic-rk3588_defconfig";
      filesToInstall = [
        "u-boot.itb"
        "idbloader.img"
        "u-boot-rockchip.bin"
      ];
    };
    # ```
    #  $ $(nix-build --no-out-link --attr rockchiprs)/bin/rockusb download-boot $(nix-build --no-out-link --attr generic-rk3588-maskrom-uploadable)/*.bin
    # ```
    generic-rk3588-maskrom-uploadable = make-maskrom-uploadable {
      u-boot = generic-rk3588;
    };


    ## NOTE: borrowing an aarch64 rockchip build still...
    #generic-rv1126 = (make-64bit-rockchip-u-boot "ubootRock64" {}).override {
    #  defconfig = "generic-rv1126_defconfig";
    #  filesToInstall = [
    #    "u-boot.itb"
    #    "idbloader.img"
    #    "u-boot-rockchip.bin"
    #  ];
    #  meta.platforms = [
    #    "armv7l-linux"
    #  ];
    #};

    generic-rv1126 =
      let
        drv =
          pkgs.pkgsCross.armv7l-hf-multiplatform.buildUBoot {
            #defconfig = "generic-rv1126_defconfig";
            #defconfig = "sonoff-ihost-rv1126_defconfig";
            defconfig = "neu2-io-rv1126_defconfig";
            extraConfig = ''
              CONFIG_SPL_RAM_DEVICE=y
              CONFIG_SPL_RAM_SUPPORT=y
            '';
            extraMeta.platforms = [ "armv7l-linux" ];
            filesToInstall = [
              #"u-boot.itb"
              "idbloader.img"
              "u-boot-rockchip.bin"
              "spl/u-boot-spl.bin"
              #"tpl/u-boot-tpl.bin"
              "u-boot-rockchip-maskrom.bin"
            ];
            # XXX probably broken?
            # But where would I get the TEE?
            TEE="/dev/null";
            #TEE=/Users/samuel/SBCs/rockchip/rkbin/bin/rv11/rv1126_tee_ta_v2.16.bin;
            ROCKCHIP_TPL=/Users/samuel/SBCs/rockchip/rkbin/bin/rv11/rv1126_ddr_924MHz_v1.14.bin;
          }
        ;
      in
      u-boot-for drv
    ;
    generic-rv1126-maskrom-uploadable = make-maskrom-uploadable {
      u-boot = generic-rv1126;
    };

    #sonoff-ihost-rv1126 =
    #  let
    #    drv =
    #      pkgs.pkgsCross.armv7l-hf-multiplatform.buildUBoot {
    #        defconfig = "sonoff-ihost-rv1126_defconfig";
    #        #extraConfig = ''
    #        #  CONFIG_SPL_RAM_DEVICE=y
    #        #  CONFIG_SPL_RAM_SUPPORT=y
    #        #'';
    #        extraMeta.platforms = [ "armv7l-linux" ];
    #        filesToInstall = [
    #          "u-boot.itb"
    #          "idbloader.img"
    #          "u-boot-rockchip.bin"
    #          "spl/u-boot-spl.bin"
    #          #"u-boot-rockchip-maskrom.bin"
    #        ];
    #        TEE="/dev/null";
    #      }
    #    ;
    #  in
    #  u-boot-for drv
    #;


    inherit pkgs;
  }

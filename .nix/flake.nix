{
  description = "ESPHome NX3L4051 development environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in
      {
        devShells.default = pkgs.mkShell {
          buildInputs = with pkgs; [
            esphome
            esptool
          ];

          shellHook = ''
            echo "ESPHome NX3L4051 development environment loaded"
            echo "ESPHome version: $(esphome version)"
          '';
        };
      }
    );
}

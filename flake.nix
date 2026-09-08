{
  inputs = {
    flake-parts.url = "github:hercules-ci/flake-parts";
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs =
    inputs@{ flake-parts, nixpkgs, ... }:
    flake-parts.lib.mkFlake { inherit inputs; } {
      systems = [ "x86_64-linux" ];
      perSystem =
        { system, ... }:
        let
          pkgs = import nixpkgs {
            inherit system;
            config = {
              allowUnfree = true;
              cudaForwardCompat = false;
              cudaCapabilities = [ "6.1" ];
            };
          };

          commonPackages = with pkgs; [
            cmake
            just
            perf
            nixfmt
            prettier
            shfmt
          ];
        in
        {
          devShells = {
            # CPU only
            default = pkgs.mkShell { packages = commonPackages; };

            # CUDA (GTX 1050)
            cuda = pkgs.mkShell {
              packages =
                commonPackages
                ++ (with pkgs.cudaPackages; [
                  cuda_nvcc
                  cuda_cudart
                ]);
            };
          };
        };
    };
}

{
  pkgs ? import <nixpkgs> {
    config = {
      allowUnfree = true;
      cudaForwardCompat = false;
      cudaCapabilities = [ "6.1" ];
    };
  },
  enableCuda ? false,
}:

let
  cuda = pkgs.cudaPackages;

  commonPackages = with pkgs; [
    cmake
    just
    perf
    nixfmt
    prettier
    shfmt
  ];

  cudaPackages = with cuda; [
    cuda_nvcc
    cuda_cudart
  ];
in

pkgs.mkShell {
  packages = commonPackages ++ pkgs.lib.optionals enableCuda cudaPackages;
}

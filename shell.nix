{
  pkgs ? import <nixpkgs> { },
}:

pkgs.mkShell {
  packages = with pkgs; [
    cmake
    nixfmt
    prettier
    shfmt
    perf
    just
  ];
}

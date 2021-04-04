{ pkgs ? import <nixpkgs> { } }:
with pkgs;
stdenv.mkDerivation {
  name = "dhteensy-1.0";
  src = ./.;
  buildInputs = [
    pkgsCross.avr.buildPackages.gcc # for `avr-gcc`
    teensy-loader-cli # to program the device
  ];
}

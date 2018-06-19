{ pkgs ? import <nixpkgs> {} }:
with pkgs;
stdenv.mkDerivation {
  name = "dhteensy-1.0";
  src = ./.;
  buildInputs = [
    avrbinutils # for the link script
    avrgcc # for `avr-gcc`
    avrlibc # for the headers needed
    teensy-loader-cli
  ];
  shellHook = ''
    export CFLAGS+=" -I${avrlibc}/avr/include"
    export CFLAGS+=" -L${avrlibc}/avr/lib/avr51"
    cp ${avrlibc}/avr/lib/avr51/crtat90usb1286.o . # XXX linker can't find this otherwise FIXME
  '';
}

with import <nixpkgs> {};
stdenv.mkDerivation {
    name = "routine";
    buildInputs = [ qt5Full ];
}

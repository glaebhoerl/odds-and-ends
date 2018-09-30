with import <nixpkgs> {};
stdenv.mkDerivation {
    name = "aliasing-qtapp";
    buildInputs = [ stdenv gcc qt5Full ];
}

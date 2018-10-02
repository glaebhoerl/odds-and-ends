with import <nixpkgs> {};
stdenv.mkDerivation {
    name = "aliasing-plasmawallpaper";
    buildInputs = [ stdenv gcc kdeFrameworks.extra-cmake-modules kdeFrameworks.plasma-framework kdeFrameworks.kwindowsystem ];
}

{ pkgs ? import <nixpkgs> {} }:
pkgs.mkShell {
  buildInputs = [ pkgs.jekyll ];
  shellHook = "jekyll serve";
}

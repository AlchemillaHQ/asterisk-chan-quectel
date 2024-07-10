/*
    Print value of AST_BUILDOPT_SUM macro
    More info:

    - Module was not compiled with the same compile-time options as this version of Asterisk
      http://itgala.xyz/module-was-not-compiled-with-the-same-compile-time-options-as-this-version-of-asterisk-2/
*/

#include <stdio.h>

#include <asterisk/buildopts.h>

void main() { puts(AST_BUILDOPT_SUM); }

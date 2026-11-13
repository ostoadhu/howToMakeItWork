the goal is to create an application, that runs in a standalone manner.
it is in a debian 2 environment.
it is written in cpp. according to at least cpp23 standard.
it uses CMake to create the build structure, and make for the build files.
it uses gcc for compilation.

the goal of the application is to interact with browser screens.
for the interaction it will rely on the xdotool toolset, to gather information about browser graphical layout.
it will emulate keystrokes and mouse clicks using xtodool. 
it will capture screenshots with imagemagick.
it will analyse screenshot with tesseract.

use cases:
the use cases are kept in a single place: the_app_AI/docs/use_cases.md


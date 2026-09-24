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

usecase 1:
we start the app from a terminal, with an argument.
the argument is the url of a webpage.
the app launches a chromium browser, opens the corresponding webpage.
takes a screenshot of the content of the webpage in the browser window.
opens the screenshot with gimp.


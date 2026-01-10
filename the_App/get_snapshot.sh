#!/bin/bash

# Start chromium in the background
chromium "tippmixpro.hu" &> /dev/null &

# Wait for the chromium window to appear and get its ID
WINDOW_ID=$(xdotool search --sync --onlyvisible --name "Tipp")

# Check if a window ID was found
if [ -z "$WINDOW_ID" ]; then
  echo "Error: Could not find window with name 'Tipp'"
  exit 1
fi

# Take a snapshot of the window
import -window "$WINDOW_ID" /home/debian/Desktop/chromium_snapshot.png

echo "Snapshot saved to /home/debian/Desktop/chromium_snapshot.png"

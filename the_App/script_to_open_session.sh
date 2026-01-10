setsid chromium "tippmixpro.hu" &> /dev/null &
xdotool sleep 1
xdotool search --sync --onlyvisible --name "Tipp" > /home/debian/Desktop/chromium_ID.txt
# end of script

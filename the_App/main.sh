#!/bin/bash
source ~/.bashrc

# global variables
window_ID=0;

function IsNodePackageManagerInstalled()
{
	if ! command -v npm &> /dev/null; then
		echo "Error: NodePackageManager is not installed."
		echo "    Consider : sudo apt install nodejs npm"
		exit 1
	else
	    NPM_VERSION=$(npm --version)
	    echo "npm version : $NPM_VERSION"
	fi
}

function IsNodeInstalled()
{
	if ! command -v node &> /dev/null; then
		echo "Error: Node is not installed."
		echo "    Consider : sudo apt install nodejs npm"
		exit 1
    else
		NODE_VERSION=$(node --version)
		echo "node version : $NODE_VERSION"
	fi
}

function IsNodeVersionManagerInstalled()
{
	if ! command -v nvm &> /dev/null; then
		echo "Error: NodeVersionManager is not installed."
		echo "    Consider : sudo apt install curl -y"
		echo "    curl https://raw.githubusercontent.com/creationix/nvm/master/install.sh | bash "
		exit 1
	else
		NVM_VERSION=$(nvm --version)
		echo "nvm version : $NVM_VERSION"
	fi
}

function isGeminiCliInstalled()
{
	if ! command -v gemini &> /dev/null; then
		echo "Error: Gemini is not installed."
		echo "    Consider :sudo npm install -g @google/gemini-cli"
		exit 1
	else
	    echo "Gemini seems to be working, check sucessful login !"
	fi
}

function open_chromium_session()
{
    setsid chromium "tippmixpro.hu" &> /dev/null &
    xdotool sleep 1
    xdotool search --sync --onlyvisible --name "Tipp" > /home/debian/Desktop/chromium_ID.txt
    window_ID=$(xdotool search --sync --onlyvisible --name "Tipp" | head -n 1)
}

function create_snapshot()
{
    echo "$window_ID"
    xdotool sleep 4;
    import -window "$window_ID" /home/debian/Desktop/snapshot.png
}

function get_window_coordinates()
{
    eval $(xdotool getwindowgeometry --shell "$window_ID");
    echo "X : $X"
    echo "Y : $Y"
    echo "WIDTH : $WIDTH"
    echo "HEIGHT : $HEIGHT"
}

# arguments : 1.x 2.y
function click_in_window()
{
    xdotool windowfocus "$window_ID";
    xdotool windowactivate "$window_ID";
    xdotool mousemove $1 $2 click 1;
}

#arguments : prompt as string
function call_gemini()
{
    PROMPT=$1
    echo "$PROMPT"
    RESULT=$(echo "$PROMPT" | gemini)
    echo "$RESULT"    
}

# argumens : 1.haystack 2.needle
function getNeedleImageCoordinates()
{
	echo "haystack_image : -$1-"
	echo "needle_image : -$2-"
    RESULT=$(compare -metric RMSE -subimage-search "$1" "$2" null:)
    echo "$RESULT"
}

########################################################################
########################################################################
rm /home/debian/Desktop/chromium_ID.txt;
rm /home/debian/Desktop/snapshot.png;
echo "";

#open_session;
#create_snapshot;
#get_window_coordinates;

PROMPT="There is an output.tsv file on the Desktop. Analyse it. 
Find összes elutasítása button content. 
If found, answer only with the coordinates of the button in a pattern like X,Y,WIDTH,HEIGHT. 
If not found respond with -1,-1,-1,-1"

#call_gemini "$PROMPT"

# getNeedleImageCoordinates "/home/debian/Desktop/snapshot_2.png" "/home/debian/Desktop/ref_login.png"

IsNodeInstalled
IsNodePackageManagerInstalled
IsNodeVersionManagerInstalled
isGeminiCliInstalled



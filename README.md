Bitaxe Monitoring Bot Tutorial
This tutorial guides you through setting up an ESP32-based Telegram bot to monitor two
Bitaxe mining devices. The bot provides real-time status updates, daily reports, and
share statistics, with a customizable command interface. Follow these steps to configure
the code and set up the Telegram bot.
Prerequisites
●
Hardware:
○
ESP32 development board (e.g., ESP32 DevKit V1)
○
Two Bitaxe devices connected to your local network
●
Software:
○
Arduino IDE (latest version recommended)
○
A Telegram account
●
Libraries:
○
Install the following libraries in Arduino IDE via Sketch > Include
Library > Manage Libraries:
■ WiFi (pre-installed with ESP32 board support)
■ HTTPClient (pre-installed with ESP32 board support)
■ ArduinoJson (version 6.x recommended)
■ UniversalTelegramBot (latest version by Brian Lough)
●
Network:
○
WiFi network credentials (SSID and password)
○
IP addresses of your Bitaxe Supra and Ultra devices
Step 1: Create a Telegram Bot
1. Open Telegram and Start BotFather:
○
Search for @BotFather in Telegram and start a chat by sending /start.
2. Create a New Bot:
○
Send /newbot to BotFather.
○
Follow the prompts to name your bot (e.g., BitaxeMonitorBot). The name
must end with "Bot"
.
○
BotFather will provide a BOT
_
TOKEN (e.g.,
123456789:AAF-XXXXXXXXXXXXXXXXXXXXXXXXXXX). Save this token.
3. Get Your Chat ID:
○
Start a chat with your new bot by sending any message (e.g.,
"Hi").
○
Open a web browser and enter:
https://api.telegram.org/bot<YOUR
_
BOT
_
TOKEN>/getUpdates
■ Replace <YOUR
_
BOT
_
TOKEN> with the token from BotFather.
○
Look for the "chat":{"id":...} field in the JSON response. The number
(e.g., -565655535) is your CHAT
_
ID. Save it.
Step 2: Set Up the Arduino Project
1. Open Arduino IDE:
○
Create a new sketch (File > New).
2. Create the Main Code File:
○
Copy and paste the following code into the main .ino file (e.g., name it
BitaxeMonitor.ino):
3. Create the Configuration File:
○
In the Arduino IDE, click the small arrow next to the sketch name and
select New Tab.
○
Name the new file config.h.
○
Copy and paste the following code into config.h:
4. Configure config.h:
○
Replace the placeholder values with your own:
■ YOUR
_
WIFI
_
SSID: Your WiFi network name.
■ YOUR
_
WIFI
_
PASSWORD: Your WiFi password.
■ YOUR
_
BOT
_
TOKEN: The token from BotFather (e.g.,
123456789:AAF-XXXXXXXXXXXXXXXXXXXXXXXXXXX).
■ YOUR
_
CHAT
_
ID: Your Telegram chat ID (e.g., -5454534425).
■ 192.168.1.X: The IP address of your Bitaxe Supra.
■ 192.168.1.Y: The IP address of your Bitaxe Ultra.
○
Save the file.
Step 3: Compile and Upload the Code
1. Select the ESP32 Board:
○
In Arduino IDE, go to Tools > Board > ESP32 Arduino and select your
ESP32 board (e.g., ESP32 Dev Module).
2. Set the Port:
○
Connect your ESP32 to your computer via USB.
○
Go to Tools > Port and select the COM port for your ESP32.
3. Compile the Code:
○
Click Sketch > Verify/Compile to check for errors. If errors occur, ensure
all libraries are installed and the config.h syntax is correct.
4. Upload the Code:
○
Click Sketch > Upload to upload the code to your ESP32.
○
Open the Serial Monitor (Tools > Serial Monitor, 115200 baud) to
verify:
ESP32 Booting...
Connecting to Wi-Fi...
○
Connected to Wi-Fi! IP: 192.186.1.Z
○
Check your Telegram chat for the message: "ESP32 started - Monitoring
My Bitaxe Supra & Ultra"
.
Step 4: Configure Telegram Bot Commands
1. Access BotFather:
○
Go back to your chat with @BotFather.
2. Set the Command List:
○
Send /mybots and select your bot.
○
Send /setcommands.
○
Paste the following list exactly as shown (no / at the start of each
command):
start - Start monitoring
stop - Pause monitoring
reset1 - Restart My Bitaxe Supra
reset2 - Restart My Bitaxe Ultra
sessionbest1 - Show session BestDiff for Supra
sessionbest2 - Show session BestDiff for Ultra
status1 - Current status of Supra
status2 - Current status of Ultra
report - Daily report with total hashrate
shares - Show accepted and rejected shares
epic
_
on - Enable epic share notifications
epic
_
off - Disable epic share notifications
○
epic
_
history - Epic shares history
○
BotFather will respond with: Success! Command list updated.
3. Verify the Menu:
○
Go to your bot’s chat in Telegram.
○
Type / and confirm the command menu appears with all 13 commands.
Step 5: Test the Bot
1. Start Monitoring:
○
Send /start to the bot. You should receive: "Monitoring activated"
.
2. Check Shares:
○
Send /shares. You should see a
report like: ``` Shares
Report
My Bitaxe Supra Accepted: X |
Rejected: Y
My Bitaxe Ultra
Accepted: Z | Rejected: W
Total Shares: (X+Z) | (Y+W)
3. Get a Daily Report:
○
Send /report. You should see:
```
Daily Report
My Bitaxe Supra Hashrate: X GH/s |
Temp: Y°C | BestDiff: Z Accepted
Shares: A | Rejected Shares: B
My Bitaxe Ultra
Hashrate: W GH/s | Temp:
V°C | BestDiff: U
Accepted Shares: C |
Rejected Shares: D
Total Hashrate: (X+W) GH/s Total Shares: (A+C) | (B+D)
4. Test Other Commands:
○
Try /status1, /reset2, /epic
_
on, etc., to ensure all features work as
expected.
Troubleshooting
●
Bot Doesn’t Connect:
○
Check the Serial Monitor for errors. If you see "certificate verify failed,
"
replace client.setCACert(TELEGRAM
_
CERTIFICATE
_
ROOT) with
client.setInsecure() in setup() as a temporary workaround, then seek
an updated certificate.
○
Verify WiFi credentials and Bitaxe IPs in config.h.
●
Commands Don’t Appear:
○
Ensure the command list was set correctly in BotFather without / at the
start of each command.
○
Wait a few minutes for Telegram to sync the changes.
●
API Issues:
○
The code assumes your Bitaxe API endpoint /api/system/info returns
fields like hashRate, temp, bestDiff, bestSessionDiff, sharesAccepted,
and sharesRejected. If the field names differ, adjust the code accordingly.
Features
●
Monitoring: Tracks hashrate, temperature, and session BestDiff for both Bitaxe
devices.
●
Alerts: Notifies you of high temperatures, low hashrate (with auto-restart), and
epic shares (BestDiff ≥ 100M).
●
Reports: Provides daily reports every 6 hours with total hashrate and shares.
●
Shares: Shows accepted/rejected shares for each Bitaxe and their totals.
●
Commands: 13 customizable commands for controlling and querying the bot.
Final Notes
Your Bitaxe Monitoring Bot is now fully operational! Share this tutorial with your group,
and feel free to tweak the code (e.g., adjust thresholds or add features) to suit your
needs. Enjoy your mining insights!

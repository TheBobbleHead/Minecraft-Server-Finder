# Minecraft Server Finder

## How to Build
paste the asio repo in a folder titled 'Dependacies'(in the root of project folder) http://think-async.com/Asio/ rename the asio folder to 'asio-1.24.0'
make sure that your build is x64 and that c++ standard is set to 14


## How to run
create a text file with all of the ip ranges you want to search.
Here is an example of a ip range 0.0.0.0-255.255.255.255(don't use this!)
make sure you keep each range on seperate lines without spaces
use https://suip.biz to find ip ranges (Use ad block to get rid of annoying ads).

In the command line type: ./(executable name) (number of threads) (path to range file) 
while the program is running it will log all found servers to FoundServers.txt in the same directory
as the program. You can also connect to localhost on port 80 to load an html page with all the servers the program has found

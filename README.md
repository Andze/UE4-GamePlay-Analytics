# UE4-GamePlay-Analytics

## Gameplay Analystic Tool 

## Description
Open GL tool for visualising Heatmap and Trajectory data from UE4 Battery Game

### Generating Logs 

1. Download the Battery tutorial game from GitHub found here: https://github.com/Andze/Unreal-4-Battery-Game
2. Open the project in Unreal Engine 4.13 or higher
3. On Loaded, Select the Simulate button located above the game view
4. when Playing the game a "Logs" folder will be generated in the directory holding all log files generated from a single session

### Using the Tool

1. If you have not already downloaded this project, you can do so from: https://github.com/Andze/UE4-GamePlay-Analytics
2. Make sure that Conan.io and CMake 3.0 or higher are installed on the machine
3. run the "Build-Solution.bat" file within the project directory
4. Open the visual studio .sln file found in the Build directory now created
5. once loaded, select Play or hit F5 to build the solution to be run

### Loading Log Files

1. Locate the desired Player or Battery Positional data log file using file explorer
2. Drag and drop the file onto the tool while it is running
3. Use navigation below to toggle off and on features
4. Add any additional Positional data log files you want to view

### Navigation and Controls 

 - Toggle corresponding Trajectories on/off using
	- 1 Key 
	- 2 Key
	- 3 Key
	- 4 Key
	- 5 Key

 - Toggle corresponding Heatmaps on/off using
	- 6 Key
	- 7 Key
	- 8 Key
	- 9 Key
	- 0 Key
 - Use the + key to add heatmaps to the aggregated Heatmap and - to take away

 - Move the camera using 
	- W (Forward)
	- A (Left)
	- S (Backwards)
	- D (Right)
	- Q (Down)
	- E (UP)

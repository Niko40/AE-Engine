
# AE Engine

This is a sbub of a 3D game engine that is designed from ground up to use Vulkan API to render everything. ( Name might change later )
Currently this is Windows only but no Windows specific code is used so porting this to other platforms should be easy.

!!!Currently this engine is in it's early stages and will not actually render anything so it is useless except as an example!!!
What currently is working are the 2 resource managers that handle file resources and device resources

This engine is licenced under MIT licence, this basically means you can freely use any part or entire
engine in your own projects for as long as you contribute me.
Full licence can be found in Licence.txt file in the solution directory

## Getting started
This project serves as a starting point of a 3D game engine but at this time it does not produce any visible results, you can doodle around some though, especially if you're curious about Vulkan and how you could use it yourself.

You will need to download some external libraries, check "External required libraries" section below for more info what you need, where you can get it and where you should unpack it in your system.

Vulkan SDK can be installed anywhere but default installation path is highly recommended.

GLFW version needs to be 3.2.1 or above. You also need to compile it yourself for Visual Studio 2017. You can download cmake here: https://cmake.org/
Wherever you unpacked this project, go to it's parent folder and create a new folder "External libraries" in it, extract GLFW in that folder.
To compile start with cmake, open a command prompt or power-shell and navigate to "...\External libraries\glfw-3.2.1" and type in these commands
```
mkdir VS2017_64
cd VS2017_64
cmake "Visual Studio 15 2017 Win64" ..
```
After this you should open the GLFW.sln file in "...\External libraries\glfw-3.2.1\VS2017_64" folder and compile both Debug and Release solution configurations as normal.

After this you can open the AE.sln file in visual studio and compile.
Have fun doodling around.

---
#### Libraries
##### Embedded in solution:

<table>
	<tr>
		<th>Library name </th>
		<th>URL </th>
	</tr>
	<tr>
		<th>Lua </th>
		<th>https://www.lua.org/ </th>
	</tr>
	<tr>
		<th>TinyXML </th>
		<th>http://www.grinninglizard.com/tinyxml2/ </th>
	</tr>
	<tr>
		<th>stb_libs </th>
		<th>https://github.com/nothings/stb#stb_libs </th>
	</tr>
	<tr>
		<th>glm </th>
		<th>http://glm.g-truc.net/0.9.8/index.html </th>
	</tr>
</table>

##### External required libraries:

<table>
	<tr>
		<th>Library name </th>
		<th>URL </th>
		<th>Default install path </th>
	</tr>
	<tr>
		<th>Vulkan SDK </th>
		<th>https://vulkan.lunarg.com/ </th>
		<th>C:\VulkanSDK </th>
	</tr>
	<tr>
		<th>GLFW 3.2.1 </th>
		<th>http://www.glfw.org/index.html </th>
		<th>'Solution folder'..\External Libraries\glfw-3.2.1 </th>
	</tr>
</table>

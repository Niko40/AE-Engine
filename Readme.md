
# AE Engine

This is a sbub of a game engine that is designed from ground up to use Vulkan API to render everything. ( Name might change later )
Currently this is Windows only but no Windows specific code is used so porting this to other platforms should be easy.

!!!Currently this engine is in it's early stages and will not actually render anything so it is useless except as an example!!!
What currently is working are the 2 resource managers that handle file resources and device resources

This engine is licenced under MIT licence, this basically means you can freely use any part or entire
engine in your own projects for as long as you contribute me.
Full licence can be found in Licence.txt file in the solution directory

#### Libraries
---
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
		<th>GLFW 3.2 </th>
		<th>http://www.glfw.org/index.html </th>
		<th>C:\VulkanSDK </th>
	</tr>
</table>

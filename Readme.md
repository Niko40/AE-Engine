
AE Engine
===

This is a sbub of a game engine that is designed from ground up to use Vulkan API to render everything. ( Name might change later )
Currently this is Windows only but no Windows specific code is used so porting this to other platforms should be easy.

!!!Currently this engine is in it's early stages and will not actually render anything so it is useless except as an example!!!
What currently is working are the 2 resource managers that handle file resources and device resources

This engine is licenced under MIT licence, this basically means you can freely use any part or entire
engine in your own projects for as long as you contribute me.
Full licence can be found in Licence.txt file in the solution directory

Libraries
---

Embedded in solution:
Library name  | URL
------------- | --------------
Lua           | https://www.lua.org/
TinyXML-2     | http://www.grinninglizard.com/tinyxml2/
stb_libs      | https://github.com/nothings/stb#stb_libs
glm           | http://glm.g-truc.net/0.9.8/index.html

External required libraries:
Library name | URL                            | Default install path
------------ | ------------------------------ | -------------
Vulkan SDK   | https://vulkan.lunarg.com/	  | C:\VulkanSDK
GLFW 3.2     | http://www.glfw.org/index.html | C:\VulkanSDK

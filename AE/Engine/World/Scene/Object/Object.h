#pragma once

#include "../../../BUILD_OPTIONS.h"
#include "../../../Platform.h"

#include "../SceneNode.h"

namespace AE
{

class SceneNode_Object : public SceneNode
{
public:
					SceneNode_Object( Engine * engine, SceneManager * scene_manager, DescriptorPoolManager * descriptor_pool_manager, const Path & scene_node_path, SceneNodeBase::Type scene_node_type );
	virtual			~SceneNode_Object();

protected:
	// because it's easier to call parent functions than branching child object's functions
	// I have inverted the parsing into sections, ParseConfigFile() is responsible for calling it's immediate
	// parent class parse function and the function will parse that section of the XML file as well as any below
	// returns new parent xml element for the next stage or nullptr if error
	tinyxml2::XMLElement	*	ParseConfigFile_ObjectLevel();
	ResourcesLoadState			CheckResourcesLoaded_ObjectLevel();
	bool						FinalizeResources_ObjectLevel();

private:

};

}

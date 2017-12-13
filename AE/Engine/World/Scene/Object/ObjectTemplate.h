#pragma once

/*

///////////////////////////////////////////////////////////
// into: .h
///////////////////////////////////////////////////////////

#include "../../../../BUILD_OPTIONS.h"
#include "../../../../Platform.h"

#include "../Object.h"

namespace AE
{

class SceneNode_ : public SceneNode_Object
{
public:
	SceneNode_( Engine * engine, SceneManager * scene_manager, DescriptorPoolManager * descriptor_pool_manager, Path & scene_node_path );
	~SceneNode_();

private:
	void							Update_Animation();
	void							Update_Logic();

	bool							ParseConfigFile();
	ResourcesLoadState				CheckResourcesLoaded();
	bool							FinalizeResources();
};

}



///////////////////////////////////////////////////////////
// into: .cpp
///////////////////////////////////////////////////////////

#include ".h"

namespace AE
{

SceneNode_::SceneNode_( Engine * engine, SceneManager * scene_manager, DescriptorPoolManager * descriptor_pool_manager, Path & scene_node_path )
	: SceneNode_Object( engine, scene_manager, descriptor_pool_manager, scene_node_path, SceneBase::Type::CAMERA )
{
}

SceneNode_::~SceneNode_()
{
}

void SceneNode_::Update_Animation()
{
	// Update all animation data
	// Copy all animation data into Vulkan system RAM bound buffer objects
}

void SceneNode_::Update_Logic()
{
	// Update logic: AI, scripts, animation changes or loops
}

bool SceneNode_::ParseConfigFile()
{
	assert( config_file->IsResourceReadyForUse() );		// config file resource should have been loaded before this function is called

	return ParseConfigFileHelper( ParseConfigFile_<Parent>Level(), "THIS_LEVEL_TYPE_HERE", [ this ]() {
		// Parse this level stuff here
		return true;
	} );
}

SceneBase::ResourcesLoadState SceneNode_::CheckResourcesLoaded()
{
	return CheckResourcesLoadedHelper( CheckResourcesLoaded_<Parent>Level(), [ this ]() {
		// check requested resources on shape level
		return SceneBase::ResourcesLoadState::READY;
	} );
}

bool SceneNode_::FinalizeResources()
{
	if( Finalize_<Parent>Level() ) {
		// finalize this level stuff
		return true;
	}
	return false;
}


}


*/
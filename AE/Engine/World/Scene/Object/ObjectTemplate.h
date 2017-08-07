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
	void							Update();
	bool							ParseConfigFile();
	ResourcesLoadState				CheckResourcesLoaded();
	bool							Finalize();
};

}



///////////////////////////////////////////////////////////
// into: .cpp
///////////////////////////////////////////////////////////

#include ".h"

namespace AE
{

SceneNode_::SceneNode_( Engine * engine, SceneManager * scene_manager, DescriptorPoolManager * descriptor_pool_manager, Path & scene_node_path )
	: SceneNode_Object( engine, scene_manager, descriptor_pool_manager, scene_node_path, SceneNodeBase::Type::CAMERA )
{
}

SceneNode_::~SceneNode_()
{
}

void SceneNode_::Update()
{
}

bool SceneNode_::ParseConfigFile()
{
	assert( config_file->IsResourceReadyForUse() );		// config file resource should have been loaded before this function is called

	auto object_level	= ParseConfigFile_ObjectLevel();
	if( object_level ) {
		auto this_level	= object_level->FirstChildElement( "THIS_LEVEL_TYPE_HERE" );
		if( nullptr != this_level ) {
			// Parse this level stuff

			return true;
		}
	}
	return false;
}

SceneNodeBase::ResourcesLoadState SceneNode_::CheckResourcesLoaded()
{
	auto object_level	= CheckResourcesLoaded_ObjectLevel();
	if( object_level == ResourcesLoadState::READY ) {
		// check requested resources on this level
		return ResourcesLoadState::READY;
	}
	return object_level;
}

bool SceneNode_::Finalize()
{
	if( Finalize_ObjectLevel() ) {
		// finalize this level stuff
		return true;
	}
	return false;
}


}


*/
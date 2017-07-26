#pragma once

// memory handling
#include "Memory/Memory.h"

// main engine components
#include "Engine.h"

// logger
#include "Logger/Logger.h"

// filesystem
#include "FileSystem/FileSystem.h"

// file resources
#include "FileResource/FileResourceManager.h"
#include "FileResource/FileResource.h"
#include "FileResource/RawData/FileResource_RawData.h"
#include "FileResource/Lua/FileResource_Lua.h"
#include "FileResource/XML/FileResource_XML.h"
#include "FileResource/Image/FileResource_Image.h"
#include "FileResource/Mesh/FileResource_Mesh.h"

// renderer
#include "Renderer/Renderer.h"
#include "Renderer/DescriptorSet/DescriptorPoolManager.h"
#include "Renderer/DescriptorSet/DescriptorSetHandle.h"

// device memory
#include "Renderer/DeviceMemory/DeviceMemoryManager.h"

// device resources
#include "Renderer/DeviceResource/DeviceResourceManager.h"
#include "Renderer/DeviceResource/DeviceResource.h"

// Add device resource includes here
#include "Renderer/DeviceResource/Mesh/DeviceResource_Mesh.h"
#include "Renderer/DeviceResource/Image/DeviceResource_Image.h"

// map and scene management
#include "World/World.h"
#include "World/Scene/SceneManager.h"
#include "World/Scene/Scene.h"
#include "World/Scene/SceneNode.h"

#include "World/Scene/Object/Shape/Shape.h"

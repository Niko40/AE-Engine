#pragma once

#include "../BUILD_OPTIONS.h"
#include "../Platform.h"

#include "../Memory/MemoryTypes.h"
#include "../Vulkan/Vulkan.h"

namespace AE
{

enum class QueueAvailability : uint32_t
{
	UNDEFINED,

	F3_PR1_SR2_PT3,		// 3 families, primary render on family 1, secondary render on family 2, primary transfer on family 3

	F2_PR1_SR1_PT2,		// 2 families, primary render on family 1, secondary render on family 1, primary transfer on family 2
	F2_PR1_SR2_PT2,		// 2 families, primary render on family 1, secondary render on family 2, primary transfer on family 2
	F2_PR1_SR2_PT1,		// 2 families, primary render on family 1, secondary render on family 2, primary transfer on family 1
	F2_PR1_SR2,			// 2 families, primary render on family 1, secondary render on family 2
	F2_PR1_PT2,			// 2 families, primary render on family 1, primary transfer on family 2

	F1_PR1_SR1_PT1,		// 1 families, primary render on family 1, secondary render on family 1, primary transfer on family 1
	F1_PR1_PT1,			// 1 families, primary render on family 1, primary transfer on family 1
	F1_PR1_SR1,			// 1 families, primary render on family 1, secondary render on family 1
	F1_PR1,				// 1 families, primary render on family 1
};

enum class UsedQueuesFlags : uint32_t
{
	PRIMARY_RENDER		= 1 << 0,
	SECONDARY_RENDER	= 1 << 1,
	PRIMARY_TRANSFER	= 1 << 2,
};

struct SharingModeInfo
{
	VkSharingMode		sharing_mode;
	Vector<uint32_t>	shared_queue_family_indices;
};

UsedQueuesFlags operator|( UsedQueuesFlags f1, UsedQueuesFlags f2 );
UsedQueuesFlags operator&( UsedQueuesFlags f1, UsedQueuesFlags f2 );
UsedQueuesFlags operator|=( UsedQueuesFlags f1, UsedQueuesFlags f2 );

}

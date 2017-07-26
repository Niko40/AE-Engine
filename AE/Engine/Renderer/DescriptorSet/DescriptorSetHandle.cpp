
#include "DescriptorSetHandle.h"

#include <utility>

#include "DescriptorPoolManager.h"

namespace AE
{

DescriptorSetHandle::DescriptorSetHandle( Engine * engine, DescriptorPoolManager * descriptor_pool, DescriptorSubPoolInfo * descriptor_pool_info, vk::DescriptorSet descriptor_set )
{
	p_engine				= engine;
	pool					= descriptor_pool;
	pool_info				= descriptor_pool_info;
	set						= descriptor_set;
	assert( p_engine );
	assert( pool );
	assert( pool_info );
	assert( set );
}

DescriptorSetHandle::DescriptorSetHandle()
{
}

DescriptorSetHandle::DescriptorSetHandle( nullptr_t )
{
}

DescriptorSetHandle::DescriptorSetHandle( DescriptorSetHandle && other )
{
	Swap( std::forward<DescriptorSetHandle>( other ) );
}

DescriptorSetHandle::~DescriptorSetHandle()
{
	if( set ) {
		pool->FreeDescriptorSet( pool_info, set );
		pool_info			= nullptr;
	}
}

DescriptorSetHandle & DescriptorSetHandle::operator=( DescriptorSetHandle && other )
{
	Swap( std::forward<DescriptorSetHandle>( other ) );
	return *this;
}

DescriptorSetHandle::operator vk::DescriptorSet()
{
	return set;
}

void DescriptorSetHandle::Swap( DescriptorSetHandle && other )
{
	std::swap( p_engine, other.p_engine );
	std::swap( pool, other.pool );
	std::swap( pool_info, other.pool_info );
	std::swap( set, other.set );
}

}

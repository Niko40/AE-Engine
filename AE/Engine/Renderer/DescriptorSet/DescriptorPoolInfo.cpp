#include "DescriptorPoolInfo.h"

namespace AE
{

bool DescriptorSubPoolInfo::operator==( DescriptorSubPoolInfo other )
{
	return pool == other.pool;
}

}

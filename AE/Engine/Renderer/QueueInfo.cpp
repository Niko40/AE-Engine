
#include "QueueInfo.h"

namespace AE
{

UsedQueuesFlags operator|( UsedQueuesFlags f1, UsedQueuesFlags f2 )
{
	return UsedQueuesFlags( uint32_t( f1 ) | uint32_t( f2 ) );
}

UsedQueuesFlags operator&( UsedQueuesFlags f1, UsedQueuesFlags f2 )
{
	return UsedQueuesFlags( uint32_t( f1 ) & uint32_t( f2 ) );
}

UsedQueuesFlags operator|=( UsedQueuesFlags f1, UsedQueuesFlags f2 )
{
	return f1 | f2;
}

}

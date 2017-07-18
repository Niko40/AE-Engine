
#include "Math.h"

namespace AE
{

size_t RoundToAlignment( size_t src_value, size_t alignment_value )
{
	return ( ( src_value / alignment_value + !!( src_value % alignment_value ) ) * alignment_value );
}

}

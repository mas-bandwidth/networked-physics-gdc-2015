#include "virtualgo/Board.h"
#include "core/Core.h"

namespace virtualgo
{
    vec3f Board::GetPointPosition( int row, int column )
    {
        CORE_ASSERT( row >= 1 );
        CORE_ASSERT( column >= 1 );
        CORE_ASSERT( row <= size );
        CORE_ASSERT( column <= size );

        const float w = config.cellWidth;
        const float h = config.cellHeight;
        const float z = config.thickness;

        const int n = ( GetSize() - 1 ) / 2;

        return vec3f( ( -n + column - 1 ) * w, ( -n + row - 1 ) * h, z );
    }
}

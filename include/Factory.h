#ifndef PROTOCOL_FACTORY_H
#define PROTOCOL_FACTORY_H

#include "Common.h"

namespace protocol
{
    template <typename T> class Factory
    {
    public:

        typedef std::function<T*()> create_function;
        
        Factory()
        {
            max_type = 0;
        }

        void Register( int type, create_function const & function )
        {
            create_map[type] = function;
            max_type = std::max( type, max_type );
        }

        bool TypeExists( int type )
        {
            auto itor = create_map.find( type );
            return itor != create_map.end();
        }

        T * Create( int type )
        {
            auto itor = create_map.find( type );
            assert( itor != create_map.end() );
            if ( itor != create_map.end() )
                return itor->second();
            else
                return nullptr;
        }

        int GetMaxType() const
        {
            return max_type;
        }

    private:

        int max_type;
        
        // todo: don't use std::map
        std::map<int,create_function> create_map;
    };
}

#endif

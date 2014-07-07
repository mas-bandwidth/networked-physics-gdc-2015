#ifndef PROTOCOL_FACTORY_H
#define PROTOCOL_FACTORY_H

#include "Common.h"

// todo: replace this!
#include <map>

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

        virtual ~Factory() {}

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

        virtual T * Create( int type )
        {
            auto itor = create_map.find( type );
            PROTOCOL_ASSERT( itor != create_map.end() );
            if ( itor != create_map.end() )
            {
                T * obj = itor->second();
                PROTOCOL_ASSERT( obj );
                PROTOCOL_ASSERT( obj->GetType() == type );
                return obj;
            }
            else
                return nullptr;
        }

        int GetMaxType() const
        {
            return max_type;
        }

    private:

        int max_type;

        std::map<int,create_function> create_map;

        Factory( const Factory & other );
        Factory & operator = ( const Factory & other );
    };
}

#endif

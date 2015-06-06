// Protocol Library - Copyright (c) 2008-2015, Glenn Fiedler

#ifndef PROTOCOL_OBJECT_H
#define PROTOCOL_OBJECT_H

namespace protocol
{
    class Object
    {  
    public:

        virtual ~Object() {}

        virtual void SerializeRead( class ReadStream & stream ) = 0;

        virtual void SerializeWrite( class WriteStream & stream ) = 0;

        virtual void SerializeMeasure( class MeasureStream & stream ) = 0;
    };
}

#define PROTOCOL_SERIALIZE_OBJECT( stream )                                                     \
    void SerializeRead( class protocol::ReadStream & stream ) { Serialize( stream ); };         \
    void SerializeWrite( class protocol::WriteStream & stream ) { Serialize( stream ); };       \
    void SerializeMeasure( class protocol::MeasureStream & stream ) { Serialize( stream ); };   \
    template <typename Stream> void Serialize( Stream & stream )

#endif

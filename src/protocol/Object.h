// Protocol Library - Copyright (c) 2014, The Network Protocol Company, Inc.

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

#endif

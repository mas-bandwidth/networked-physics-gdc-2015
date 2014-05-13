/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#ifndef PROTOCOL_MESSAGE_H
#define PROTOCOL_MESSAGE_H

#include "Common.h"
#include "Stream.h"

namespace protocol
{
    class Message : public Object
    {
    public:

        Message( int type ) : m_type(type) {}

        int GetId() const {  return m_id; }
        int GetType() const { return m_type; }

        void SetId( uint16_t id ) { m_id = id; }

    private:
      
        uint16_t m_id;  
        uint16_t m_type;       
    };

    typedef vector<uint8_t> Block;

    class BlockMessage : public Message
    {
    public:

        BlockMessage() : Message(0) {}

        BlockMessage( shared_ptr<Block> block ) : Message(0) { m_block = block; }

        virtual void Serialize( Stream & stream ) { assert(false); }

        shared_ptr<Block> GetBlock() { return m_block; }

    private:

        shared_ptr<Block> m_block;
    };
}

#endif

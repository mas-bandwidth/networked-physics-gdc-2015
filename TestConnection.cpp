// todo: Implement test connection

#if 0

void test_connection()
{
    cout << "test_connection" << endl;

    Connection::Config config;

    config.packetType = PACKET_Connection;
    config.maxPacketSize = 4 * 1024;

    Connection connection( config );

    const int NumAcks = 100;

    for ( int i = 0; i < NumAcks*2; ++i )
    {
        auto packet = connection.WritePacket();

        connection.ReadPacket( packet );

        if ( connection.GetCounter( Connection::PacketsAcked ) == NumAcks )
            break;
    }

    assert( connection.GetCounter( Connection::PacketsAcked ) == NumAcks );
    assert( connection.GetCounter( Connection::PacketsWritten ) == NumAcks + 1 );
    assert( connection.GetCounter( Connection::PacketsRead ) == NumAcks + 1 );
    assert( connection.GetCounter( Connection::PacketsDiscarded ) == 0 );
    assert( connection.GetCounter( Connection::ReadPacketFailures ) == 0 );
}

#endif

// serialize test

#include <string>
#include <vector>
#include <limits>
#include <stdexcept>

namespace protocol
{
    class serialize_exception : public std::exception
    {
        // ...
    };

    struct StreamValue
    {
        int64_t value;         // the actual value to be serialized
        int64_t min;           // the minimum value
        int64_t max;           // the maximum value
    };

    enum StreamMode
    {
        STREAM_Read,
        STREAM_Write
    };

    class Stream
    {
        Stream( StreamMode mode )
        {
            SetMode( mode );
        }

        void SetMode( StreamMode mode )
        {
            m_mode = mode;
            m_index = 0;
        }

        bool IsReading() const 
        {
            return m_mode == STREAM_Read;
        }

        bool IsWriting() const
        {
            return m_mode == STREAM_Write;
        }

        void SerializeInteger( int64_t value, int64_t min, int64_t max )
        {
            if ( m_mode == STREAM_Write )
            {
                // todo: pop value off the read values -- verify min/max matches

                // todo: if stream min/max doesn't match then throw exception

                // todo: if value read is outside bounds -- throw exception (it has been tampered with)
            }
            else
            {
                // todo: assert value is within min/max (programmer error if not)

                // todo: push value/min/max
            }
        }

        StreamMode m_mode;
        int m_index;
        std::vector<Value> m_values;
    };

    class Atom
    {  
        virtual void Serialize( Stream & stream ) = 0;
    };
}

// ...

/*

#include <stdexcept>
#include <limits>
#include <iostream>
 
using namespace std;
class MyClass
{
public:
   void MyFunc(char c)
   {
      if(c < numeric_limits<char>::max())
         throw invalid_argument("MyFunc argument too large.");
      //...
   }
};

int main()
{
   try
   {
      MyFunc(256); //cause an exception to throw
   }
 
   catch(invalid_argument& e)
   {
      cerr << e.what() << endl;
      return -1;
   }
   //...
   return 0;
}

*/

/*

#include <memory>
#include <vector>
// ...
// circle and shape are user-defined types
auto p = make_shared<circle>( 42 );
vector<shared_ptr<shape>> v = load_shapes();

for_each( begin(v), end(v), [&]( const shared_ptr<shape>& s ) {
    if( s && *s == *p )
        cout << *s << " is a match\n";
} );

*/
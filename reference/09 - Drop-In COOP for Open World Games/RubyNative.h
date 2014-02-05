// Ruby Native by Glenn Fiedler
// http://www.gaffer.org

#ifndef RUBY_NATIVE_H
#define RUBY_NATIVE_H

#include <vector>
#include <string>
#include <assert.h>
#include <ruby/ruby.h>


/// Ruby integer utility class

class RubyInt
{
public:

    static bool check(VALUE object)
    {
        // check if int
        return FIXNUM_P(object)!=0;
    }

    static int internal(VALUE object)
    {
        assert(check(object));
        return NUM2INT(object);
    }

    static VALUE create(int number)
    {
        return INT2FIX(number);
    }
};


/// Ruby long utility class

class RubyLong
{
public:

    static bool check(VALUE object)
    {
        // check if long
        return FIXNUM_P(object)!=0;
    }

    static long internal(VALUE object)
    {
        assert(check(object));
        return NUM2LONG(object);
    }

    static VALUE create(long number)
    {
        return LONG2FIX(number);
    }
};


/// Ruby float utility class

class RubyFloat
{
public:

    static bool check(VALUE object)
    {
        // check if float or int (safety)
        return rb_obj_is_kind_of(object, rb_cFloat)!=0 || FIXNUM_P(object)!=0;
    }

    static float internal(VALUE object)
    {
        assert(check(object));
        return (float) NUM2DBL(object);
    }

    static VALUE create(float number)
    {
        return rb_float_new(number);
    }
};


/// Ruby double utility class

class RubyDouble
{
public:

    static bool check(VALUE object)
    {
        // check if float or int (safety)
        return rb_obj_is_kind_of(object, rb_cFloat)!=0 || FIXNUM_P(object)!=0;
    }

    static double internal(VALUE object)
    {
        assert(check(object));
        return NUM2DBL(object);
    }

    static VALUE create(double number)
    {
        return rb_float_new(number);
    }
};


/// Ruby string utility class

class RubyString
{
public:

    static bool check(VALUE object)
    {
        // check if string
        return rb_obj_is_kind_of(object, rb_cString)!=0;
    }

    static const char* internal(VALUE object)
    {
        assert(check(object));
        return STR2CSTR(object);
    }

    static VALUE create(const char buffer[])
    {
        return rb_str_new2(buffer);
    }
};


/// Ruby array utility class

class RubyArray
{
public:

    static bool check(VALUE object)
    {
        // check if array
        return rb_obj_is_kind_of(object, rb_cArray)!=0;
    }

    static int size(VALUE object)
    {
        // get array size
        assert(check(object));
        return RARRAY(object)->len;
    }

    static VALUE element(VALUE object, int index)
    {
        // get array element at index
        assert(check(object));
        return rb_ary_entry(object, index);
    }
};


/// Ruby hash utility class

#include <ruby/st.h>

class RubyHash
{
public:

    static bool check( VALUE object )
    {
        // check if hash
        return rb_obj_is_kind_of( object, rb_cHash ) != 0;
    }

    static int size(VALUE object)
    {
        // get hash entries
        assert( check(object) );
		return INT2FIX( RHASH(object)->tbl->num_entries );
    }

    static VALUE lookup( VALUE object, VALUE key )
    {
        // get hash value for key
        assert( check( object ) );
		return rb_hash_aref( object, key );
    }
};


/// Ruby utility methods

class RubyUtility
{
protected:

    static VALUE defineClass(const char className[], VALUE baseClass=rb_cObject)
    {
        return rb_define_class(className, baseClass);
    }

    static void addMethod(VALUE rubyClass, const char methodName[], void *functionPointer, int arguments)
    {
        rb_define_method(rubyClass, methodName, (VALUE (*)(...))functionPointer, arguments);
    }

    static void addStaticMethod(VALUE rubyClass, const char methodName[], void *functionPointer, int arguments)
    {
        rb_define_singleton_method(rubyClass, methodName, (VALUE (*)(...))functionPointer, arguments);
    }

    static void addConstant(VALUE rubyClass, const char constantName[], int value)
    {
        rb_define_const(rubyClass, constantName, RubyInt::create(value));
    }
};


/// Manages an c++ object wrapped inside a ruby object.
/// Intended for wrapping concrete C++ classes such as vectors, matrices etc.

template <class T> class RubyObject : public RubyUtility
{
public:

    /// check if ruby object is of this type

    static bool check(VALUE object)
    {
        return rb_obj_is_kind_of(object, rubyClass()) != 0;
    }

    /// get internal c++ object in ruby object

    static T* internal(VALUE object)
    {
        assert(check(object));

        T *internal;
        Data_Get_Struct(object, T, internal);
        assert(internal);

        return internal;
    }

    /// get static ruby class

    static VALUE& rubyClass()
    {
        static VALUE staticRubyClass;
        return staticRubyClass;
    }

protected:

    /// construct internal c++ object wrapped inside ruby object.

    static T* construct(VALUE &object)
    {
        T *internal;
        object = Data_Make_Struct(rubyClass(), T, 0, 0, internal);
        return internal;
    }

    RubyObject() {}
    RubyObject(const RubyObject &other) {}
};


/// Complex object manages an outer ruby object and an inner c++ object.
///
/// The life cycle of the ruby object is managed by ruby's mark and sweep garbage collector.
///
/// The c++ object is reference counted so that when one complex object references another
/// we can be sure that the c++ object is not deleted while in use.
///
/// Once the ruby outer object has been freed, and the c++ inner object has been released, 
/// the wrapping complex object can be deleted safely.

class ComplexRubyObjectInterface
{
public:

    /// virtual destructor

    virtual ~ComplexRubyObjectInterface() {}

    /// add reference to object

    virtual void reference() = 0;

    /// release reference to object

    virtual void release() = 0;

    /// get number of c++ object references

    virtual int references() const = 0;

    /// get outer ruby object

    virtual VALUE outer() const = 0;
};


/// Complex Ruby object manager.
/// Manages instances of complex ruby objects and deletes them when they are no longer referenced.

class ComplexRubyObjectManager
{
    typedef std::vector<ComplexRubyObjectInterface*> Objects;

public:

    static ComplexRubyObjectManager& instance()
    {
        static ComplexRubyObjectManager singleton;
        return singleton;
    }

    void add(ComplexRubyObjectInterface *object)
    {
        assert(object);
        objects.push_back(object);
    }

    void update()
    {
        Objects::iterator i = objects.begin();
        
        while (i!=objects.end())
        {
            ComplexRubyObjectInterface *object = *i;
            if (!object->references())
            {
                delete object;
                i = objects.erase(i);
            }
            else
                ++i;
        }
    }

private:

    Objects objects;

    ComplexRubyObjectManager() {};
    ComplexRubyObjectManager(const ComplexRubyObjectManager &other);
};


/// Complex object implementation template.
/// Manages operations for a specific inner c++ type.

template <class T> class ComplexRubyObject : public ComplexRubyObjectInterface, public RubyUtility
{
public:

    /// constructor

    ComplexRubyObject(T *inner)
    {
        _references = 1;
        _outer = Qnil;
        _inner = inner;

        // add to object manager

        ComplexRubyObjectManager::instance().add(this);
    }

    /// destructor.
    /// checks references are all accounted for

    virtual ~ComplexRubyObject()
    {
        assert(!_references);
        assert(!_inner);
    }

    /// get inner object (c++)

    virtual T* inner()
    {
        return _inner;
    }

    /// get outer object (ruby)

    virtual VALUE outer() const
    {
        return _outer;
    }

    /// check if ruby object is of this type

    static bool check(VALUE object)
    {
        VALUE outer = rb_iv_get(object, "@outer");

        return rb_obj_is_kind_of(outer, rubyClass()) != 0;
    }

    /// get internal c++ object in ruby object

    static T* internal(VALUE object)
    {
        assert(check(object));

        VALUE outer = rb_iv_get(object, "@outer");

        ComplexRubyObject<T> *complexRubyObject;
        Data_Get_Struct(outer, ComplexRubyObject<T>, complexRubyObject);

        assert(complexRubyObject);
        assert(complexRubyObject->inner());

        return complexRubyObject->inner();
    }

    /// get internal complex object in ruby object

    static ComplexRubyObject<T>* complex(VALUE object)
    {
        assert(check(object));

        VALUE outer = rb_iv_get(object, "@outer");

        ComplexRubyObject<T> *complexRubyObject;
        Data_Get_Struct(outer, super, complexRubyObject);
        assert(complexRubyObject);

        return complexRubyObject;
    }

public:

    /// add dependency to another complex object

    virtual void dependency(ComplexRubyObjectInterface *complexRubyObject)
    {
        complexRubyObject->reference();
        _dependencies.push_back(complexRubyObject);
    }

public:

    /// mark ruby objects we depend on

    virtual void mark()
    {
        try
        {
            // mark dependencies

            for (unsigned int i=0; i<_dependencies.size(); i++)
                rb_gc_mark(_dependencies[i]->outer());
        }
        catch (...)
        {
            printf("exception in %s mark", type());
            exit(1);
        }
    }

    /// free ruby object as a result of ruby garbage collection

    virtual void free()
    {
        try
        {
            assert(_outer!=Qnil);
            _outer = Qnil;

            release();
        }
        catch (...)
        {
            printf("exception in %s free", type());
            exit(1);
        }
    }

    /// free c++ object because there are no more references to it

    virtual void released()
    {
        // delete inner c++ object

        try
        {
            assert(_inner);
            delete _inner;
            _inner = 0;
        }
        catch (...)
        {
            printf("exception in %s delete inner", type());
            exit(1);
        }

        // release dependencies

        try
        {
            for (unsigned int i=0; i<_dependencies.size(); i++)
                _dependencies[i]->release();
        }
        catch (...)
        {
            printf("exception in %s dependency release", type());
            exit(1);
        }

        _dependencies.clear();

        // update object manager

        ComplexRubyObjectManager::instance().update();
    }

    /// add a reference to inner object

    virtual void reference()
    {
        assert(_references>0);
        _references++;
    }

    /// release reference to inner object

    virtual void release()
    {
        assert(_references>0);

        _references--;

        if (!_references)
            released();
    }

    /// get number of references to inner object

    virtual int references() const
    {
        assert(_references>=0);
        return _references;
    }

    /// get static ruby class

    static VALUE& rubyClass()
    {
        static VALUE staticRubyClass;
        return staticRubyClass;
    }

protected:

    /// static mark method called from ruby garbage collector.
    /// used to redirect to instance mark method.

    static void staticMark(ComplexRubyObject<T> *complexRubyObject)
    {
        assert(complexRubyObject);
        complexRubyObject->mark();
    }

    /// static free method called from ruby garbage collector.
    /// used to redirect to instance free method.

    static void staticFree(ComplexRubyObject<T> *complexRubyObject)
    {
        assert(complexRubyObject);
        complexRubyObject->free();
    }

    /// set ruby outer object.

    void setOuter(VALUE outer)
    {
        _outer = outer;
    }

    /// wrap complex object in ruby object.
    /// creates a new ruby object "outer" and sets as rubyObject @outer member data.

    static void wrap(VALUE object, ComplexRubyObject<T> *complexRubyObject)
    {
        VALUE outer = Data_Wrap_Struct(rubyClass(), staticMark, staticFree, complexRubyObject);

        complexRubyObject->setOuter(outer);

        rb_iv_set(object, "@outer", outer);
    }

    /// super type definition for subclasses

    typedef ComplexRubyObject<T> super;

private:

    /// get type name used for error reporting

    const char* type() const
    {
        return rb_class2name(rubyClass());
    }

    int _references;                                            ///< reference count to c++ object
    VALUE _outer;                                               ///< ruby outer object
    T* _inner;                                                  ///< c++ inner object
    std::vector<ComplexRubyObjectInterface*> _dependencies;     ///< array of dependencies.
};


/// An error class with a text title and description

class RubyError
{
    std::string _message;       ///< error message string.

public:

    /// construct error with given message.

	RubyError(const char message[])
	{
		_message = message;
	}
	
	/// virtual dtor
	
	virtual ~RubyError() {}

    /// get error title.
    /// if you want to define a new error type, derive from this class
    /// and return an identifying string in the override of this method.

	virtual const char * title() const
	{
		return "Native Error";
	}

    /// get error message.

	virtual const char * message() const
	{
		return _message.c_str();
	}
};


// some helper macros to make everything nice

#define RubyMethod static VALUE

#define DefineClass(name) rubyClass() = defineClass(name);
#define AddStaticMethod(name, method, arguments) addStaticMethod(rubyClass(), name, (void*)method, arguments);
#define AddMethod(name, method, arguments) addMethod(rubyClass(), name, (void*)method, arguments);

#define RUBY_BEGIN(method) const char currentRubyMethod[]=method; try { 
#define RUBY_END(...) } catch (RubyError &error) { char buffer[256]; sprintf(buffer, "%s: %s - %s", currentRubyMethod, error.title(), error.message()); rb_raise(rb_eRuntimeError, buffer); return Qnil; } catch (...) { rb_raise(rb_eRuntimeError, "%s: unknown c++ exception", currentRubyMethod); return Qnil; }

#define RUBY_INT(x)	  RubyInt::internal(x)
#define RUBY_FLOAT(x) RubyFloat::internal(x)
#define RUBY_BOOLEAN(x) x == Qtrue ? true : false

#define RUBY_SYMBOL(x) ID2SYM( rb_intern( x ) )

#define RUBY_HASH_LOOKUP(hash,key) RubyHash::lookup(hash,RUBY_SYMBOL(key))

#define RUBY_NEW(cls) rb_class_new_instance( 0, NULL, rb_const_get( rb_cObject, rb_intern( cls ) ) )
#define RUBY_NEW_ARGS(cls,argc,argv) rb_class_new_instance( argc, argv, rb_const_get( rb_cObject, rb_intern( cls ) ) )

/// Ruby Interpreter class for running scripts

class RubyInterpreter
{
public:

    RubyInterpreter::RubyInterpreter(int argc, char **argv)
    {
        #ifdef _WIN32
        NtInitialize(&argc, &argv);
        #endif
    	
        ruby_init();
        ruby_script("Ruby Native");
    }
    
    void RubyInterpreter::run(const char script[])
    {
        ruby_incpush(".");
        rb_load_file((char*)script);
        ruby_run();
        ruby_finalize();
    }
};

#endif

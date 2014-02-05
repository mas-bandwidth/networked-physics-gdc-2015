require 'Math'

# data types

module DataType
  Boolean = 0
  Integer = 1
  Float = 2
  Vector = 3
  UnitVector = 4
  Quaternion = 5
  Count = 6
end

# property

class Property
  attr_reader :name, :type, :default, :minimum, :maximum, :resolution, :constant
  def initialize name, type, default, minimum, maximum, resolution, constant
    raise 'default property value not specified!' if default == nil
    raise "invalid property type: #{type}" if not type.instance_of?(Fixnum) or type < 0 or type >= DataType::Count
    raise 'must supply both minimum and maximum or neither' if minimum and !maximum || maximum && !minimum
    raise 'resolution requires min/max' if resolution and not minimum
    @name = name
    @type = type
    @minimum = minimum
    @maximum = maximum
    @resolution = resolution
    @constant = constant
    if @type == DataType::Integer || @type == DataType::Float
      if @resolution
        instance_eval "def validate value \n float_quantize_clamp( value, @minimum, @maximum, @resolution ) \n end"
      elsif @minimum
        instance_eval "def validate value \n return @minimum if value < @minimum; return @maximum if value > @maximum; value \n end"
      else
        instance_eval "def validate value \n value \n end "
      end
    elsif @type == DataType::Vector
      if @resolution
        instance_eval "def validate value \n value.quantize_clamp( @minimum, @maximum, @resolution ) \n end"
      elsif @minimum
        instance_eval "def validate value \n value.clamp( @minimum, @maximum ) \n end"
      else
        instance_eval "def validate value \n value \n end"
      end
    elsif @type == DataType::UnitVector || @type == DataType::Quaternion
      instance_eval "def validate value \n value.normalize \n end"
    else
      instance_eval "def validate value \n value \n end"
    end
    @default = validate( default )
  end
  def instance
    return @default if @default == true || @default == false || @default.instance_of?( Fixnum ) || @default.instance_of?( Float )
    @default.dup
  end
  def parse string
    # todo: raise exception on parse error
    if @type == DataType::Boolean
      return true if string == 'true'
      return false
    elsif @type == DataType::Integer
      return string.to_i
    elsif @type == DataType::Float
      return string.to_f
    elsif @type == DataType::Vector || @type == DataType::UnitVector
      return Vector.new( $1.to_f, $2.to_f, $3.to_f ) if string.scan /\((.+),(.+),(.+)\)/
    elsif @type == DataType::Quaternion
      return Quaternion.new( $1.to_f, $2.to_f, $3.to_f, $4.to_f ) if string.scan /\((.+),(.+),(.+),(.+)\)/
    end
  end
  def to_s
    "#{@name} -> #{@default.to_s}"
  end
end

# property set

module PropertySet
  def define_properties
    class_eval "@@propertyHash = Hash.new\n def #{name}.propertyHash\n @@propertyHash\n end"
    class_eval "@@propertyArray = Array.new\n def #{name}.propertyArray\n @@propertyArray\n end"
    class_eval "def propertyHash\n @@propertyHash\n end"
    class_eval "def propertyArray\n @@propertyArray\n end"
  end
  def property name, attributes
    property = Property.new( name, attributes[:type], attributes[:default], attributes[:minimum], attributes[:maximum], attributes[:resolution], attributes[:constant] )
    property.freeze
    propertyArray.push property
    propertyHash[name] = property
    class_eval "attr_reader :#{name}"
    define_method( "#{name}=" ) { |block| value = block; property = propertyHash[name]; instance_variable_set( "@#{name}", property.validate( value ) ) }
  end
  def property_definition name
    propertyHash[name]
  end
end

# property container

class PropertyContainer
  extend PropertySet
  def initialize *args
    propertyArray.each do |property|
      instance_variable_set( "@#{property.name}", propertyHash[property.name].instance )
    end
  end
  def get_property name
    instance_variable_get( "@#{name}" )
  end
  def set_property name, value
    property = propertyHash[name]
    raise "attempt to set unknown property '#{name}'" if property == nil
    instance_variable_set( "@#{name}", property.validate( value ) )
  end
  def set_properties hash
    hash.each_pair do |name,value|
      set_property name, value
    end
  end
  def lock_properties
    # todo: once locked, constant properties may not be changed
  end
  def serialize stream
    propertyArray.each do |property|
      next if property.constant
      write_value = get_property property.name
      read_value = stream.serialize write_value
      set_property property.name, read_value
    end
  end
end

class TestPropertyContainer < PropertyContainer
  define_properties
  property :constantPropertyA,   { :type => DataType::Boolean,    :default => 1, :constant => true }
  property :constantPropertyB,   { :type => DataType::Boolean,    :default => 2, :constant => true }
  property :constantPropertyC,   { :type => DataType::Boolean,    :default => 3, :constant => true }
  property :booleanProperty,     { :type => DataType::Boolean,    :default => false }
  property :integerProperty,     { :type => DataType::Integer,    :default => 10 }
  property :floatProperty,       { :type => DataType::Float,      :default => 0.0, :minimum => 0.0, :maximum => 1.0, :resolution => 0.01 }
  property :vectorProperty,      { :type => DataType::Vector,     :default => Vector.new(0,0,0) }
  property :unitVectorProperty,  { :type => DataType::UnitVector, :default => Vector.new(0,0,1) }
  property :quaternionProperty,  { :type => DataType::Quaternion, :default => Quaternion.new(1,0,0,0) }
end

def test_property_container
  properties = TestPropertyContainer.propertyArray
  properties.each do |property|
    puts property.to_s
  end
end

test_property_container if __FILE__ == $0

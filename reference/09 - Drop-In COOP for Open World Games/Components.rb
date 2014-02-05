# object model test

require 'Math'
require 'Properties'

class NetStream
  Read = 0
  Write = 1
  def initialize
    @mode = Write
    @stack = Array.new
  end
  def mode= value
    @mode = value
  end
  def reading?
    @mode == Read
  end
  def writing?
    @mode == Write
  end
  def serialize value
    if @mode == Write
      @stack.push value
    else
      value = @stack[0]
      @stack.delete value
    end
    value
  end
end

# base component

class Component < PropertyContainer
  def update deltaTime
  end
  def hibernate
  end
  def unhibernate
  end
  def print
    puts self
    propertyArray.each do |property|
      puts " + #{property.name} = #{get_property(property.name)} [#{property.default}]"
    end    
  end
end

class ComponentObject
  attr_reader :id, :type, :components
  def initialize id, type, components
    @id = id
    @type = type
    @components = components
  end
  def hibernate
    @components.each { |component| component.hibernate }
  end
  def unhibernate
    @components.each { |component| component.unhibernate }
  end
  def update deltaTime
    @components.each { |component| component.update deltaTime }
  end
  def print
    puts "object #{@id} (#{type})"
    @components.each do |component| 
      puts " + #{component}"
      component.propertyArray.each do |property|
        puts "   + #{property.name} = #{component.get_property(property.name)} [#{property.default}]"
      end    
    end
  end
end

class ComponentObjectSchema
  attr_reader :componentMap, :objectComponents, :componentNames, :objectTypes
  def initialize
    @componentMap = declare_component_map 
    @objectComponents = declare_object_components
    @componentNames = @componentMap.keys.sort { |a,b| a.to_s <=> b.to_s }
    @objectTypes = @objectComponents.keys.sort { |a,b| a.to_s <=> b.to_s }
  end
  def print
    puts 'component map:'
    @componentNames.each do |componentName|
      puts " + #{componentName} -> #{@componentMap[componentName]}"
    end
    puts 'object components:'
    @objectTypes.each do |objectType|
      puts " + #{objectType} -> [#{@objectComponents[objectType].join(',')}]"
    end
  end
  def lookup_component_by_name object, name
    components = @objectComponents[object.type]
    index = components.index name
    return object.components[index] if index != nil
    nil
  end
  private 
  def declare_component_map
    {
      :a => ComponentA,
      :b => ComponentB,
      :c => ComponentC,
      :d => ComponentD
    }
  end
  def declare_object_components
    {
      :a => [ :a, :c, :d ],
      :b => [ :b, :c ]
    }
  end
end

require "rexml/document"

class ComponentObjectBuilder
  def initialize schema
    @schema = schema
  end
  def buildObject id, type
    componentNames = @schema.objectComponents[type]
    raise "invalid object type #{type}" if componentNames == nil
    components = componentNames.map { |componentName| cls = @schema.componentMap[componentName]; cls.new }
    ComponentObject.new id, type, components
  end
  def load filename
    objectsHash = Hash.new
    file = File.new( filename )
    xml = REXML::Document.new file
    objects = xml.elements['objects']
    raise "cannot find 'objects' xml section in #{filename}" if objects == nil
    objects.elements.each do |object|
      raise 'no object id' if object.attributes['id'] == nil
      objectId = object.attributes['id'].to_i
      objectType = object.attributes['type'].to_sym
      objectInstance = buildObject objectId, objectType
      raise "duplicate object id #{objectId}" if objectsHash[objectId] != nil
      objectsHash[objectId] = objectInstance
      raise 'invalid object' if not objectInstance
      object.elements.each { |component| loadComponent objectInstance, component }
    end
    return objectsHash
  end
  def save filename, objects
    document = REXML::Document.new 
    objectsElement = REXML::Element.new 'objects'
    document.add_element objectsElement
    objects.each_pair do |id,object|
      objectElement = REXML::Element.new 'object'
      objectElement.attributes['id'] = id
      objectElement.attributes['type'] = object.type
      objectsElement.add_element objectElement
      objectComponents = @schema.objectComponents[object.type]
      index = 0
      object.components.each do |component|
        componentName = objectComponents[index]
        saveComponent component, componentName, objectElement
        index += 1
      end
    end
    file = File.new( filename, 'w+' )
    document.write file, 2
  end
  private
  def saveComponent component, componentName, objectElement
    componentElement = REXML::Element.new 'component'
    componentElement.attributes['name'] = componentName
    componentElement.attributes['version'] = '1'          # todo: pass in version from component once it exists...
    objectElement.add_element componentElement
    component.propertyArray.each do |property|
      propertyElement = REXML::Element.new 'property'
      propertyElement.attributes['name'] = property.name
      propertyElement.attributes['value'] = component.get_property property.name.to_s
      componentElement.add_element propertyElement
    end
  end
  def loadComponent object, xml
    componentName = xml.attributes['name'].to_sym
    componentClass = @schema.componentMap[componentName]
    raise "invalid component name '#{name}'" if not componentClass
    componentVersion = xml.attributes['version']
    component = @schema.lookup_component_by_name object, componentName
    raise "could not find component '#{componentName}' for object #{object}" if component == nil
    propertyHash = Hash.new
    xml.elements.each do |property|
      name = property.attributes['name'].to_sym
      value = property.attributes['value']
      raise "invalid property name" if name == nil
      raise "invalid property value" if value == nil
      propertyDefinition = eval "#{componentClass}.property_definition :#{name}"
      raise "invalid property #{name}" if propertyDefinition == nil
      parsed_value = propertyDefinition.parse value
      propertyHash[name] = parsed_value
    end
    component.set_properties propertyHash
    component.lock_properties
  end
end

class TestComponent < Component
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

def test_component
  testComponent = TestComponent.new
  puts testComponent.constantPropertyA
  puts testComponent.constantPropertyB
  puts testComponent.constantPropertyC
  puts testComponent.booleanProperty
  testComponent.booleanProperty = true
  puts testComponent.booleanProperty
  puts testComponent.integerProperty
  puts testComponent.floatProperty
  puts testComponent.vectorProperty
  puts testComponent.unitVectorProperty
  puts testComponent.quaternionProperty
  testComponent.quaternionProperty = Quaternion.new(1,2,3,4)
  puts testComponent.quaternionProperty
end

def test_serialize
  stream = NetStream.new
  a = 1
  b = 2
  c = 3
  puts "in: #{a},#{b},#{c}"
  stream.serialize a
  stream.serialize b
  stream.serialize c
  stream.mode = NetStream::Read
  a = stream.serialize nil
  b = stream.serialize nil
  c = stream.serialize nil
  puts "out: #{a},#{b},#{c}"
end

def test_property_set_get
  testComponent = TestComponent.new
  puts( testComponent.get_property( :floatProperty ) )
  testComponent.set_property :floatProperty, 4.9
  puts( testComponent.get_property( :floatProperty ) )
end

def test_component_serialize
  stream = NetStream.new
  inputComponent = TestComponent.new
  inputComponent.booleanProperty = true
  inputComponent.integerProperty = 7
  inputComponent.floatProperty = 3.1415927
  inputComponent.vectorProperty = Vector.new(1,2,3)
  inputComponent.unitVectorProperty = Vector.new(1,0,0)
  inputComponent.quaternionProperty = Quaternion.new(0,1,0,0)
  inputComponent.serialize stream
  outputComponent = TestComponent.new
  stream.mode = NetStream::Read
  outputComponent.serialize stream  
  outputComponent.print
end

class ClampComponent < Component
  define_properties
  property :integerProperty,            { :type => DataType::Integer,     :default => 5,   :minimum => 0,   :maximum => 10  }
  property :floatProperty,              { :type => DataType::Float,       :default => 0.5, :minimum => 0.0, :maximum => 1.0 }
  property :quantizedFloatProperty,     { :type => DataType::Float,       :default => 0.5, :minimum => 0.0, :maximum => 1.0, :resolution => 0.1 }
  property :vectorProperty,             { :type => DataType::Vector,      :default => Vector.new(0,0,0), :minimum => Vector.new(-1,-1,-1), :maximum => Vector.new(+1,+1,+1) }
  property :quantizedVectorProperty,    { :type => DataType::Vector,      :default => Vector.new(0,0,0), :minimum => Vector.new(-1,-1,-1), :maximum => Vector.new(+1,+1,+1), :resolution => Vector.new(0.1,0.1,0.1) }
  property :unitVectorProperty,         { :type => DataType::UnitVector,  :default => Vector.new(0,0,1) }
  property :quaternionProperty,         { :type => DataType::Quaternion,  :default => Quaternion.new(1,0,0,0) }
end

def test_component_clamp
  component = ClampComponent.new
  component.integerProperty = 20
  component.floatProperty = 2.0
  component.quantizedFloatProperty = 0.1234
  component.vectorProperty = Vector.new(2,2,2)
  component.quantizedVectorProperty = Vector.new(1.245421,0.6423,0.21235)
  component.unitVectorProperty = Vector.new(-10,+5,+3)
  component.quaternionProperty = Quaternion.new(10,5,0,0)
  component.print
  component.integerProperty = -20
  component.floatProperty = -2.0
  component.quantizedFloatProperty = 0.5634
  component.vectorProperty = Vector.new(-2,-2,-2)
  component.quantizedVectorProperty = Vector.new(-1.45421,0.5423,0.35235)
  component.unitVectorProperty = Vector.new(-10,+5,+3)
  component.quaternionProperty = Quaternion.new(10,0,0,0)
  component.print
end
  
class DefaultClampComponent < Component
  define_properties
  property :integerProperty,            { :type => DataType::Integer,     :default => -10, :minimum => 0, :maximum => 10  }
  property :floatProperty,              { :type => DataType::Float,       :default => 50.0, :minimum => 0.0, :maximum => 1.0 }
  property :quantizedFloatProperty,     { :type => DataType::Float,       :default => 0.3245, :minimum => 0.0, :maximum => 1.0, :resolution => 0.1 }
  property :vectorProperty,             { :type => DataType::Vector,      :default => Vector.new(0.24553,50.0,0.1235), :minimum => Vector.new(-1,-1,-1), :maximum => Vector.new(+1,+1,+1) }
  property :quantizedVectorProperty,    { :type => DataType::Vector,      :default => Vector.new(0.1234,0.52341,0.2432), :minimum => Vector.new(-1,-1,-1), :maximum => Vector.new(+1,+1,+1), :resolution => Vector.new(0.1,0.1,0.1) }
  property :unitVectorProperty,         { :type => DataType::UnitVector,  :default => Vector.new(0,0,10) }
  property :quaternionProperty,         { :type => DataType::Quaternion,  :default => Quaternion.new(10,0,0,0) }
end
  
def test_component_default_value_clamp
  component = DefaultClampComponent.new
  component.print
end

# test component object

class ComponentA < Component
  define_properties
  property :integerProperty,   { :type => DataType::Integer,  :default => -10, :minimum => 0, :maximum => 10  }
  property :floatProperty,     { :type => DataType::Float,    :default => 0.3245, :minimum => 0.0, :maximum => 1.0, :resolution => 0.1 }
  property :vectorProperty,    { :type => DataType::Vector,   :default => Vector.new(0.1234,0.52341,0.2432), :minimum => Vector.new(-1,-1,-1), :maximum => Vector.new(+1,+1,+1), :resolution => Vector.new(0.1,0.1,0.1) }
end

class ComponentB < Component
  define_properties
  property :vectorProperty,       { :type => DataType::Vector,      :default => Vector.new(0.1234,0.52341,0.2432), :minimum => Vector.new(-1,-1,-1), :maximum => Vector.new(+1,+1,+1), :resolution => Vector.new(0.1,0.1,0.1) }
  property :quaternionProperty,   { :type => DataType::Quaternion,  :default => Quaternion.new(0,20,0,0) }
end

class ComponentC < Component
  define_properties
  property :booleanProperty,    { :type => DataType::Boolean,  :default => false }
  property :integerProperty,   { :type => DataType::Integer,  :default => 5, :minimum => 5, :maximum => 10 }
end

class ComponentD < Component
  define_properties
  property :integerProperty,      { :type => DataType::Integer,    :default => -10, :minimum => 0, :maximum => 10  }
  property :unitVectorProperty,   { :type => DataType::UnitVector, :default => Vector.new(0,0,5) }
end

def test_schema
  puts '-----------------------------------------------------'
  schema = ComponentObjectSchema.new
  schema.print
  puts '-----------------------------------------------------'
  object = ComponentObject.new 1, :a, [ ComponentA.new, ComponentB.new, ComponentC.new ]
  object.print
  puts '-----------------------------------------------------'
end

def test_object_builder
  schema = ComponentObjectSchema.new
  objectBuilder = ComponentObjectBuilder.new schema
  objects = Array.new
  objects.push objectBuilder.buildObject( 1, :a )
  objects.push objectBuilder.buildObject( 2, :b )
  objects.each do |object|
    puts '------------------------------------------------------------------------'
    puts "[object #{object.id}]"
    object.components.each { |component| component.print }
  end
  puts '------------------------------------------------------------------------'
end

def test_scene_load_and_save
  schema = ComponentObjectSchema.new
  objectBuilder = ComponentObjectBuilder.new schema
  scene = objectBuilder.load 'scene.xml'
  scene.each_pair do |id,object|
    puts '------------------------------------------------------------------------'
    puts "[object #{id}]"
    object.components.each { |component| component.print }
  end
  puts '------------------------------------------------------------------------'
  objectBuilder.save 'export.xml', scene
end

require 'mysql'

class ComponentObjectDatabase
  def initialize schema, objectBuilder, objectTable, componentTablePrefix
    @schema = schema
    @objectBuilder = objectBuilder
    @objectTable = objectTable
    @componentTablePrefix = componentTablePrefix
  end
  def connect host, database, user, password
    disconnect
    @mysql = Mysql.new( host, user, password, database )
  end
  def disconnect
    @mysql.close if @mysql
    @mysql = nil
  end
  def clear
    @mysql.query "DROP table IF EXISTS #{@objectTable}"
    @schema.componentMap.each_key do |componentName|
      @mysql.query "DROP table IF EXISTS #{@componentTablePrefix}_#{componentName}"
    end
  end
  def create
    # create object table indexed by id
    @mysql.query "CREATE TABLE #{@objectTable} ( id INT, type INT )"
    @mysql.query "CREATE UNIQUE INDEX id ON #{@objectTable} (id)"
    # create component table indexed by id for each component type
    @schema.componentMap.each_pair do |componentName,componentClass|
      @mysql.query componentTableCreateSql( componentName, componentClass )
      @mysql.query "CREATE UNIQUE INDEX id ON #{@componentTablePrefix}_#{componentName} (id)"
    end
  end
  def componentTableCreateSql componentName, componentClass
    sql  = "CREATE TABLE #{@componentTablePrefix}_#{componentName} ( id INT,"
    properties = eval "#{componentClass}.propertyArray"
    properties.each do |property|
      if property.type == DataType::Boolean
        sql += " #{property.name} ENUM('true','false'),"
      elsif property.type == DataType::Integer
        sql += " #{property.name} INT,"
      elsif property.type == DataType::Float
        sql += " #{property.name} FLOAT,"
      elsif property.type == DataType::Vector || property.type == DataType::UnitVector
        sql += " #{property.name}_x FLOAT, #{property.name}_y FLOAT, #{property.name}_z FLOAT,"
      elsif property.type == DataType::Quaternion
        sql += " #{property.name}_w FLOAT, #{property.name}_x FLOAT, #{property.name}_y FLOAT, #{property.name}_z FLOAT,"
      end
    end
    sql.chomp! ','
    sql += ' )'
    sql
  end
  def storeComponent id, componentName, component
    sql = "REPLACE INTO #{@componentTablePrefix}_#{componentName} ( id,"
    properties = component.class.propertyArray
    properties.each  do |property|
      if property.type == DataType::Boolean || property.type == DataType::Integer || property.type == DataType::Float
        sql += " #{property.name},"
      elsif property.type == DataType::Vector || property.type == DataType::UnitVector
        sql += " #{property.name}_x, #{property.name}_y, #{property.name}_z,"
      elsif property.type == DataType::Quaternion
        sql += " #{property.name}_w, #{property.name}_x, #{property.name}_y, #{property.name}_z,"
      end
    end
    sql.chomp! ','
    sql += " ) VALUES ( #{id},"
    properties.each do |property|
      value = component.get_property property.name
      if property.type == DataType::Boolean
        if value == true
          sql += " 'true',"
        else
          sql += " 'false',"
        end
      elsif property.type == DataType::Integer
        sql += " #{value.to_i},"
      elsif property.type == DataType::Float
        sql += " #{value.to_f},"
      elsif property.type == DataType::Vector || property.type == DataType::UnitVector
        sql += " #{value.x},#{value.y},#{value.z},"
      elsif property.type == DataType::Quaternion
        sql += " #{value.w},#{value.x},#{value.y},#{value.z},"
      end
    end
    sql.chomp! ','
    sql += ' )'
    @mysql.query sql
  end
  def retrieveComponent id, componentName, component
    results = @mysql.query "SELECT * FROM #{@componentTablePrefix}_#{componentName} WHERE id = #{id}"
    row = results.fetch_row
    index = 1
    component.propertyArray.each do |property|
      value = component.get_property property.name
      if property.type == DataType::Boolean || property.type == DataType::Integer || property.type == DataType::Float
        component.set_property( property.name, property.parse( row[index] ) )
        index += 1
      elsif property.type == DataType::Vector || property.type == DataType::UnitVector
        x = row[index].to_f
        y = row[index+1].to_f
        z = row[index+2].to_f
        component.set_property( property.name, Vector.new(x,y,z) )
        index += 3
      elsif property.type == DataType::Quaternion
        w = row[index].to_f
        x = row[index+1].to_f
        y = row[index+2].to_f
        z = row[index+3].to_f
        component.set_property( property.name, Quaternion.new(w,x,y,z) )
        index += 4
      end
    end
  end
  def storeObjectType id, objectType
    typeId = @schema.objectTypes.index objectType.to_sym
    raise "invalid object type '#{objectType}'" if typeId == nil
    @mysql.query "REPLACE INTO #{@objectTable} ( id, type ) VALUES ( #{id}, #{typeId} )"
  end
  def storeObject id, object
    storeObjectType id, object.type
    componentNames = @schema.objectComponents[object.type]
    index = 0
    object.components.each do |component|
      componentName = componentNames[index]
      storeComponent id, componentName, component
      index += 1
    end
  end
  def retrieveObject id, type = nil
    if type == nil
      results = @mysql.query "SELECT type FROM #{@objectTable} WHERE id = #{id} "
      typeId = results.fetch_row[0].to_i
      type = @schema.objectTypes[typeId]
    end
    object = @objectBuilder.buildObject( id, type )
    componentNames = @schema.objectComponents[object.type]
    index = 0
    object.components.each do |component|
      componentName = componentNames[index]
      retrieveComponent( id, componentName, component )
      index += 1
    end
    return object
  end
  def objects
    objectHash = Hash.new
    results = @mysql.query "SELECT id, type FROM #{@objectTable}"
    results.each do |row|
      objectHash[row[0].to_i] = @schema.objectTypes[row[1].to_i]
    end
    objectHash
  end
  def print
    puts '-------------------------------------------------------------'
    puts 'objects:'
    results = @mysql.query "SELECT id, type FROM #{@objectTable}"
    results.each do |row|
      puts " + #{row[0]} (type #{@schema.objectTypes[row[1].to_i]})"
    end
    componentNames = @schema.componentMap.keys.sort { |a,b| a.to_s <=> b.to_s }
    componentNames.each do |componentName|
      puts '-------------------------------------------------------------'
      puts "component #{componentName}:"
      results = @mysql.query "SELECT * FROM #{@componentTablePrefix}_#{componentName}"
      results.each do |row|
        puts " + #{row[0]} -> { #{row[1..-1].join(', ')} }"
      end
    end
    puts '-------------------------------------------------------------'
  end
end

def test_component_database
  begin
    schema = ComponentObjectSchema.new
    objectBuilder = ComponentObjectBuilder.new schema
    database = ComponentObjectDatabase.new schema, objectBuilder, 'objects', 'component'
    database.connect 'localhost', 'test', 'testuser', 'testpass'
    database.clear
    database.create
    database.storeComponent 1, :a, ComponentA.new
    database.storeComponent 1, :a, ComponentA.new
    database.storeComponent 2, :a, ComponentA.new
    database.storeComponent 3, :a, ComponentA.new
    database.storeComponent 4, :b, ComponentB.new
    database.storeComponent 5, :b, ComponentB.new
    database.storeComponent 1, :c, ComponentC.new
    database.storeComponent 2, :c, ComponentC.new
    database.storeComponent 5, :c, ComponentC.new
    database.storeComponent 2, :d, ComponentD.new
    database.storeComponent 3, :d, ComponentD.new
    database.storeComponent 5, :d, ComponentD.new
    database.storeObjectType 1, :a
    database.storeObjectType 2, :a
    database.storeObjectType 3, :a
    database.storeObjectType 4, :b
    database.storeObjectType 5, :b
    database.print
  rescue Mysql::Error => e
    puts "Error code: #{e.errno}"
    puts "Error message: #{e.error}"
    puts "Error SQLSTATE: #{e.sqlstate}" if e.respond_to?("sqlstate")
    raise
  ensure
    database.disconnect if database
  end
end

def test_object_store_and_retrieve
  begin
    schema = ComponentObjectSchema.new
    objectBuilder = ComponentObjectBuilder.new schema
    objects = Hash.new
    objects[1] = objectBuilder.buildObject 1, :a
    objects[2] = objectBuilder.buildObject 2, :a
    objects[3] = objectBuilder.buildObject 3, :b
    database = ComponentObjectDatabase.new schema, objectBuilder, 'objects', 'component'
    database.connect 'localhost', 'test', 'testuser', 'testpass'
    database.clear
    database.create
    objects.each_pair { |id,object| database.storeObject id, object }
    database.print
    objects = Hash.new
    objects[1] = database.retrieveObject 1
    objects[2] = database.retrieveObject 2
    objects[3] = database.retrieveObject 3
    objects.each { |id,object| object.print }
  rescue Mysql::Error => e
    puts "Error code: #{e.errno}"
    puts "Error message: #{e.error}"
    puts "Error SQLSTATE: #{e.sqlstate}" if e.respond_to?("sqlstate")
    raise
  ensure
    database.disconnect if database
  end
end

def test_database_import_export
  begin
    schema = ComponentObjectSchema.new
    objectBuilder = ComponentObjectBuilder.new schema
    import = objectBuilder.load 'scene.xml'
    database = ComponentObjectDatabase.new schema, objectBuilder, 'objects', 'component'
    database.connect 'localhost', 'test', 'testuser', 'testpass'
    database.clear
    database.create
    import.each_pair { |id,object| database.storeObject id, object }
    database.print
    export = Hash.new
    databaseObjects = database.objects
    databaseObjects.each_pair do |id,type|
      export[id] = database.retrieveObject id, type
    end
    export.each_pair { |id,object| object.print }
    objectBuilder.save 'export.xml', export
  rescue Mysql::Error => e
    puts "Error code: #{e.errno}"
    puts "Error message: #{e.error}"
    puts "Error SQLSTATE: #{e.sqlstate}" if e.respond_to?("sqlstate")
    raise
  ensure
    database.disconnect if database
  end 
end

def test_components
  test_component_default_value_clamp
  test_schema
  test_object_builder
  test_scene_load_and_save
  test_component_database
  test_object_store_and_retrieve
  test_database_import_export
end

test_components if __FILE__ == $0

class Address
  attr_reader :a, :b, :c, :d, :port
  def initialize a,b,c,d,port
    @a = a
    @b = b
    @c = c
    @d = d
    @port = port
  end
end

class NetStream
  def reading?
    false
  end
  def writing?
    true
  end
  def serializeBoolean value
    value
  end
  def serializeByte value
    value
  end
  def serializeShort value
    value
  end
  def serializeInteger value
    value
  end
  def serializeFloat value
    value
  end
end

class NetObject
  attr_accessor :ident, :priority
  def initialize ident
    @ident = ident
    @priority = 0.0
  end
  def print
    puts "net object: #{ident} -> #{priority}"
  end
end

class NetObjectSet
  def initialize
    @map = Hash.new
    @objects = Array.new
  end
  def add object
    raise 'object is nil' if object == nil
    raise 'object already added' if @map[object.ident] 
    @objects.push(object)
    @map[object.ident] = object
  end
  def remove object
    raise 'object is nil' if object == nil
    raise 'object does not exist' if @map[object.ident] == nil
    @objects.erase(object.ident)
    @map[object.ident] = nil
  end
  def sort
    @objects.sort! { |a,b| b.priority <=> a.priority }
  end
  def clear
    @objects = Array.new
    @map = Hash.new
  end
  def [] ident
    @map[ident]
  end
  def size
    @objects.size
  end
  def each
    @objects.each do |object|
      yield( object )
    end
  end
end

class Cube < NetObject
  def initialize ident
    super(ident)
    @x = 0.0
    @y = 0.0
    @z = 0.0
  end
  def print
    puts "cube #{ident}: priority = #{priority}"
  end
  def serialize stream
    @x = stream.serializeFloat @x
    @y = stream.serializeFloat @y
    @z = stream.serializeFloat @z
  end
end

def test_net
  a = Cube.new 0 
  b = Cube.new 1
  c = Cube.new 2
  a.priority = 0.5
  b.priority = 1.0
  c.priority = 2.0
  objectSet = NetObjectSet.new
  objectSet.add a
  objectSet.add b
  objectSet.add c
  objectSet.sort
  objectSet.each do |object|
    object.print
  end
  stream = NetStream.new
  c.serialize stream
end

test_net if __FILE__ == $0

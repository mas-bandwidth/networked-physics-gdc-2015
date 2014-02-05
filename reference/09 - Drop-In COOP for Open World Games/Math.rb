# math routines

def float_quantize_clamp value, minimum, maximum, resolution
  value = minimum if value < minimum
  value = maximum if value > maximum
  index = ( ( value - minimum ) / resolution + 0.000001 ).floor
  minimum + resolution * index
end

class Vector
  attr_accessor :x, :y, :z
  def initialize x,y,z
    @x = x.to_f
    @y = y.to_f
    @z = z.to_f
  end
  def clamp min, max
    @x = min.x if @x < min.x
    @x = max.x if @x > max.x
    @y = min.y if @y < min.y
    @y = max.y if @y > max.y
    @z = min.z if @z < min.z
    @z = max.z if @z > max.z
    self
  end
  def quantize_clamp min, max, resolution
    @x = float_quantize_clamp @x, min.x, max.x, resolution.x
    @y = float_quantize_clamp @y, min.y, max.y, resolution.y
    @z = float_quantize_clamp @z, min.z, max.z, resolution.z
    self
  end
  def normalize
    lengthSquared = @x*@x + @y*@y + @z*@z
    length = Math.sqrt( lengthSquared )
    inverseLength = 1.0 / length
    @x = @x * inverseLength
    @y = @y * inverseLength
    @z = @z * inverseLength
    self
  end
  def + other
    Vector.new( @x + other.x, @y + other.y, @z + other.z )
  end
  def - other
    Vector.new( @x - other.x, @y - other.y, @z - other.z )
  end
  def length
    Math.sqrt( @x*@x + @y*@y + @z*@z )
  end
  def length2d
    Math.sqrt( @x*@x + @y*@y )
  end
  def to_s
    "(#{@x},#{@y},#{@z})"
  end
end

class Quaternion
  attr_accessor :w, :x, :y, :z
  def initialize w,x,y,z
    @w = w.to_f
    @x = x.to_f
    @y = y.to_f
    @z = z.to_f
  end
  def normalize
    lengthSquared = @w*@w + @x*@x + @y*@y + @z*@z
    length = Math.sqrt( lengthSquared )
    inverseLength = 1.0 / length
    @w = @w * inverseLength
    @x = @x * inverseLength
    @y = @y * inverseLength
    @z = @z * inverseLength
    self
  end
  def to_s
    "(#{@w},#{@x},#{@y},#{@z})"
  end
end

def random_float min, max
  rand * ( max - min ) + min
end

def random_vector min, max
  Vector.new( random_float( min.x, max.x ), random_float( min.y, max.y ), random_float( min.z, max.z ) )
end

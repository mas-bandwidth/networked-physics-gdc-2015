puts "\noptimize position:\n\n"

class Position
  attr_accessor :x, :y, :z
  def initialize( x, y, z )
    @x = x
    @y = y
    @z = z
  end
end

position_values = []
position_base_values = []

position_identical = 0

File.readlines( 'scripts/position_values.txt' ).each do |line|

  values = line.split( ',' )

  position = Position.new( values[0].to_i,
                           values[1].to_i,
                           values[2].to_i )

  position_values.push position

  position_base = Position.new( values[3].to_i,
                                values[4].to_i,
                                values[5].to_i )

  if position.x == position_base.x &&
     position.y == position_base.y &&
     position.z == position_base.z
    position_identical += 1
  end

  position_base_values.push position_base

  #puts "#{position.x},#{position.y},#{position.z},#{position_base.x},#{position_base.y},#{position_base.z}"

end

class PositionBandwidthEstimate
  attr_accessor :small_bits_xy, :large_bits_xy,
                :small_bits_z, :large_bits_z,
                :total_bits
  def initialize small_bits_xy, large_bits_xy, small_bits_z, large_bits_z
    @small_bits_xy = small_bits_xy
    @large_bits_xy = large_bits_xy
    @small_bits_z = small_bits_z
    @large_bits_z = large_bits_z
    @total_bits = 0
  end
  def evaluate position_values, position_base_values
    bits = 0
    small_threshold_xy = ( 1 << ( @small_bits_xy - 1 ) ) - 1;
    large_threshold_xy = ( 1 << ( @large_bits_xy - 1 ) ) - 1 + small_threshold_xy;
    small_threshold_z = ( 1 << ( @small_bits_z - 1 ) ) - 1;
    large_threshold_z = ( 1 << ( @large_bits_z - 1 ) ) - 1 + small_threshold_z;
    position_values.each_index do |i|
      position = position_values[i]
      base_position = position_base_values[i]
      dx = position.x - base_position.x
      dy = position.y - base_position.y
      dz = position.z - base_position.z
      if dx.abs >= large_threshold_xy || dy.abs >= large_threshold_xy || dz.abs >= large_threshold_z
        bits += 1 + 18 + 18 + 14
      else
        small_x = dx.abs < small_threshold_xy
        small_y = dy.abs < small_threshold_xy
        small_z = dz.abs < small_threshold_z
        bits += 1 + 3
        if small_x
          bits += small_bits_xy
        else
          bits += large_bits_xy
        end
        if small_y
          bits += small_bits_xy
        else
          bits += large_bits_xy
        end
        if small_z
          bits += small_bits_z
        else
          bits += large_bits_z
        end
      end
    end
    @total_bits = bits
  end
  def name
    "#{small_bits_xy}-#{large_bits_xy}-#{small_bits_z}-#{large_bits_z}"
  end
end

position_bandwidth_estimates = []

for small_bits_xy in 2..6
  for large_bits_xy in 8..10
    for small_bits_z in 2..6
      for large_bits_z in 8..10
        bandwidth_estimate = PositionBandwidthEstimate.new( small_bits_xy, large_bits_xy, small_bits_z, large_bits_z )
        puts bandwidth_estimate.name
        bandwidth_estimate.evaluate position_values, position_base_values
        position_bandwidth_estimates.push bandwidth_estimate
      end
    end
  end
end

position_bandwidth_estimates.sort! { |a,b| a.total_bits <=> b.total_bits }

absolute_position_bits = ( 1 + 18 + 18 + 14 ) * position_values.size

puts
puts "num position samples: #{position_values.size}"
puts
puts "absolute position: #{absolute_position_bits} bits"
puts

for i in 0..9
  bandwidth_estimate = position_bandwidth_estimates[i]
  puts "compression #{bandwidth_estimate.name}: #{bandwidth_estimate.total_bits} (#{(bandwidth_estimate.total_bits/absolute_position_bits.to_f*100.0).round(1)})"
end

puts

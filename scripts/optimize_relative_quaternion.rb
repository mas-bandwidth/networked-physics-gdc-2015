puts "\noptimize relative quaternion:\n\n"

class RelativeQuaternion
  attr_accessor :x, :y, :z, :w
  def initialize( x, y, z, w )
    @x = x
    @y = y
    @z = z
    @w = w
  end
end

relative_quaternion_values = []

File.readlines( 'scripts/relative_quaternion_values.txt' ).each do |line|

  values = line.split( ',' )

  relative_quaternion = RelativeQuaternion.new( 255 - values[0].to_i / 2,
                                                255 - values[1].to_i / 2,
                                                255 - values[2].to_i / 2,
                                                values[3].to_i / 2 )

  relative_quaternion_values.push relative_quaternion

end

class RelativeQuaternionBandwidthEstimate
  attr_accessor :small_bits, :large_bits, :total_bits
  def initialize small_bits, large_bits
    @small_bits = small_bits
    @large_bits = large_bits
    @total_bits = 0
  end
  def evaluate relative_quaternion_values
    bits = 0
    small_threshold = ( 1 << ( @small_bits - 1 ) ) - 1;
    large_threshold = ( 1 << ( @large_bits - 1 ) ) - 1 + small_threshold;
    relative_quaternion_values.each_index do |i|
      relative_quaternion = relative_quaternion_values[i]
      dx = relative_quaternion.x
      dy = relative_quaternion.y
      dz = relative_quaternion.z
      if dx.abs >= large_threshold || dy.abs >= large_threshold || dz.abs >= large_threshold
        bits += 1 + 2 + 9 + 9 + 9
      else
        small_x = dx.abs < small_threshold
        small_y = dy.abs < small_threshold
        small_z = dz.abs < small_threshold
        bits += 1 + 3
        if small_x
          bits += small_bits
        else
          bits += large_bits
        end
        if small_y
          bits += small_bits
        else
          bits += large_bits
        end
        if small_z
          bits += small_bits
        else
          bits += large_bits
        end
      end
    end
    @total_bits = bits
  end
  def name
    "#{small_bits}-#{large_bits}"
  end
end

relative_quaternion_bandwidth_estimates = []

for small_bits in 2..6
  for large_bits in 6..10
    next if small_bits >= large_bits
    bandwidth_estimate = RelativeQuaternionBandwidthEstimate.new( small_bits, large_bits )
    puts bandwidth_estimate.name
    bandwidth_estimate.evaluate relative_quaternion_values
    relative_quaternion_bandwidth_estimates.push bandwidth_estimate
  end
end

relative_quaternion_bandwidth_estimates.sort! { |a,b| a.total_bits <=> b.total_bits }

absolute_smallest_three_bits = ( 2 + 9 + 9 + 9 ) * relative_quaternion_values.size

puts
puts "num relative quaternion samples: #{relative_quaternion_values.size}"
puts
puts "absolute smallest three: #{absolute_smallest_three_bits} bits"
puts

for i in 0..5
  bandwidth_estimate = relative_quaternion_bandwidth_estimates[i]
  puts "compression #{bandwidth_estimate.name}: #{bandwidth_estimate.total_bits} (#{(bandwidth_estimate.total_bits/absolute_smallest_three_bits.to_f*100.0).round(1)})"
end

best = relative_quaternion_bandwidth_estimates[0]

puts "\naverage bits per-relative quaternion: #{(best.total_bits/relative_quaternion_values.size.to_f).round(2)}"

puts

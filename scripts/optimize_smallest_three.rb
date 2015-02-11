puts "\noptimize smallest three:\n\n"

class SmallestThree
  attr_accessor :largest, :a, :b, :c
  def initialize( largest, a, b, c )
    @largest = largest
    @a = a
    @b = b
    @c = c
  end
end

smallest_three_values = []
smallest_three_base_values = []

File.readlines( 'scripts/smallest_three_values.txt' ).each do |line|

  values = line.split( ',' )

  smallest_three = SmallestThree.new( values[0].to_i,
                                      values[1].to_i,
                                      values[2].to_i,
                                      values[3].to_i )

  smallest_three_values.push smallest_three

  smallest_three_base = SmallestThree.new( values[4].to_i,
                                           values[5].to_i,
                                           values[6].to_i,
                                           values[7].to_i )

  if smallest_three.largest == smallest_three_base.largest &&
     smallest_three.a == smallest_three_base.a &&
     smallest_three.b == smallest_three_base.b &&
     smallest_three.c == smallest_three_base.c
    smallest_three_identical += 1
  end

  smallest_three_base_values.push smallest_three_base

end

class SmallestThreeBandwidthEstimate
  attr_accessor :small_bits, :large_bits, :total_bits
  def initialize small_bits, large_bits
    @small_bits = small_bits
    @large_bits = large_bits
    @total_bits = 0
  end
  def evaluate smallest_three_values, smallest_three_base_values
    bits = 0
    small_threshold = ( 1 << ( @small_bits - 1 ) ) - 1;
    large_threshold = ( 1 << ( @large_bits - 1 ) ) - 1 + small_threshold;
    smallest_three_values.each_index do |i|
      smallest_three = smallest_three_values[i]
      base_smallest_three = smallest_three_base_values[i]
      da = smallest_three.a - base_smallest_three.a
      db = smallest_three.b - base_smallest_three.b
      dc = smallest_three.c - base_smallest_three.c
      if smallest_three.largest != base_smallest_three.largest || da.abs >= large_threshold || db.abs >= large_threshold || dc.abs >= large_threshold
        bits += 1 + 2 + 9 + 9 + 9
      else
        small_a = da.abs < small_threshold
        small_b = db.abs < small_threshold
        small_c = dc.abs < small_threshold
        bits += 1 + 3
        if small_a
          bits += small_bits
        else
          bits += large_bits
        end
        if small_b
          bits += small_bits
        else
          bits += large_bits
        end
        if small_c
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

smallest_three_bandwidth_estimates = []

for small_bits in 2..6
  for large_bits in 8..10
    bandwidth_estimate = SmallestThreeBandwidthEstimate.new( small_bits, large_bits )
    puts bandwidth_estimate.name
    bandwidth_estimate.evaluate smallest_three_values, smallest_three_base_values
    smallest_three_bandwidth_estimates.push bandwidth_estimate
  end
end

smallest_three_bandwidth_estimates.sort! { |a,b| a.total_bits <=> b.total_bits }

absolute_smallest_three_bits = ( 2 + 9 + 9 + 9 ) * smallest_three_values.size

puts
puts "num smallest three samples: #{smallest_three_values.size}"
puts
puts "absolute smallest three: #{absolute_smallest_three_bits} bits"
puts

for i in 0..9
  bandwidth_estimate = smallest_three_bandwidth_estimates[i]
  puts "compression #{bandwidth_estimate.name}: #{bandwidth_estimate.total_bits} (#{(bandwidth_estimate.total_bits/absolute_smallest_three_bits.to_f*100.0).round(1)})"
end

puts

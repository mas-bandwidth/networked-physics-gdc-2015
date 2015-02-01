
puts "\ndelta position:\n\n"

delta_position_x = []
delta_position_y = []
delta_position_z = []
delta_position_total_x = 0
delta_position_total_y = 0
delta_position_total_z = 0
File.readlines( 'output/delta_position.txt' ).each do |line|
  values = line.split( ',' )
  position_x = values[0].to_i
  position_y = values[1].to_i
  position_z = values[2].to_i
  delta_position_x.push position_x
  delta_position_y.push position_y
  delta_position_z.push position_z
  delta_position_total_x += position_x
  delta_position_total_y += position_y
  delta_position_total_z += position_z
end
(0..10).each do |i|
  threshold = 2**i - 1
  count_x = 0
  count_y = 0
  count_z = 0
  0.upto(threshold) do |j|
    count_x += delta_position_x[j]
    count_y += delta_position_y[j]
    count_z += delta_position_z[j]
  end
  percent_x = count_x / delta_position_total_x.to_f * 100.0
  percent_y = count_y / delta_position_total_y.to_f * 100.0
  percent_z = count_z / delta_position_total_z.to_f * 100.0
  puts "  #{threshold}: #{percent_x.round(1)}% #{percent_y.round(1)}% #{percent_z.round(1)}%"
end

puts "\ndelta angle:\n\n"

delta_angle = []
delta_angle_total = 0
File.readlines( 'output/delta_angle.txt' ).each do |line|
  value = line.to_i
  delta_angle.push value
  delta_angle_total += value
end
(0..10).each do |i|
  threshold = 2**i - 1
  count = 0
  0.upto(threshold) do |j|
    count = count + delta_angle[j]
  end
  percent = count / delta_angle_total.to_f * 100.0
  puts "  #{threshold}: #{percent.round(1)}%"
end

puts "\ndelta axis:\n\n"

delta_axis_x = []
delta_axis_y = []
delta_axis_z = []
delta_axis_total_x = 0
delta_axis_total_y = 0
delta_axis_total_z = 0
File.readlines( 'output/delta_axis.txt' ).each do |line|
  values = line.split( ',' )
  axis_x = values[0].to_i
  axis_y = values[1].to_i
  axis_z = values[2].to_i
  delta_axis_x.push axis_x
  delta_axis_y.push axis_y
  delta_axis_z.push axis_z
  delta_axis_total_x += axis_x
  delta_axis_total_y += axis_y
  delta_axis_total_z += axis_z
end
(0..10).each do |i|
  threshold = 2**i - 1
  count_x = 0
  count_y = 0
  count_z = 0
  0.upto(threshold) do |j|
    count_x += delta_axis_x[j]
    count_y += delta_axis_y[j]
    count_z += delta_axis_z[j]
  end
  percent_x = count_x / delta_axis_total_x.to_f * 100.0
  percent_y = count_y / delta_axis_total_y.to_f * 100.0
  percent_z = count_z / delta_axis_total_z.to_f * 100.0
  puts "  #{threshold}: #{percent_x.round(1)}% #{percent_y.round(1)}% #{percent_z.round(1)}%"
end

puts "\ndelta axis angle:\n\n"

delta_axis_angle_x = []
delta_axis_angle_y = []
delta_axis_angle_z = []
delta_axis_angle_total_x = 0
delta_axis_angle_total_y = 0
delta_axis_angle_total_z = 0
File.readlines( 'output/delta_axis_angle.txt' ).each do |line|
  values = line.split( ',' )
  axis_angle_x = values[0].to_i
  axis_angle_y = values[1].to_i
  axis_angle_z = values[2].to_i
  delta_axis_angle_x.push axis_angle_x
  delta_axis_angle_y.push axis_angle_y
  delta_axis_angle_z.push axis_angle_z
  delta_axis_angle_total_x += axis_angle_x
  delta_axis_angle_total_y += axis_angle_y
  delta_axis_angle_total_z += axis_angle_z
end
(0..10).each do |i|
  threshold = 2**i - 1
  count_x = 0
  count_y = 0
  count_z = 0
  0.upto(threshold) do |j|
    count_x += delta_axis_angle_x[j]
    count_y += delta_axis_angle_y[j]
    count_z += delta_axis_angle_z[j]
  end
  percent_x = count_x / delta_axis_angle_total_x.to_f * 100.0
  percent_y = count_y / delta_axis_angle_total_y.to_f * 100.0
  percent_z = count_z / delta_axis_angle_total_z.to_f * 100.0
  puts "  #{threshold}: #{percent_x.round(1)}% #{percent_y.round(1)}% #{percent_z.round(1)}%"
end

puts "\ndelta smallest three:\n\n"

delta_smallest_three_x = []
delta_smallest_three_y = []
delta_smallest_three_z = []
delta_smallest_three_total_x = 0
delta_smallest_three_total_y = 0
delta_smallest_three_total_z = 0
File.readlines( 'output/delta_smallest_three.txt' ).each do |line|
  values = line.split( ',' )
  smallest_three_x = values[0].to_i
  smallest_three_y = values[1].to_i
  smallest_three_z = values[2].to_i
  delta_smallest_three_x.push smallest_three_x
  delta_smallest_three_y.push smallest_three_y
  delta_smallest_three_z.push smallest_three_z
  delta_smallest_three_total_x += smallest_three_x
  delta_smallest_three_total_y += smallest_three_y
  delta_smallest_three_total_z += smallest_three_z
end
(0..10).each do |i|
  threshold = 2**i - 1
  count_x = 0
  count_y = 0
  count_z = 0
  0.upto(threshold) do |j|
    count_x += delta_smallest_three_x[j]
    count_y += delta_smallest_three_y[j]
    count_z += delta_smallest_three_z[j]
  end
  percent_x = count_x / delta_smallest_three_total_x.to_f * 100.0
  percent_y = count_y / delta_smallest_three_total_y.to_f * 100.0
  percent_z = count_z / delta_smallest_three_total_z.to_f * 100.0
  puts "  #{threshold}: #{percent_x.round(1)}% #{percent_y.round(1)}% #{percent_z.round(1)}%"
end

puts "\ndelta quaternion:\n\n"

delta_quaternion_x = []
delta_quaternion_y = []
delta_quaternion_z = []
delta_quaternion_w = []
delta_quaternion_total_x = 0
delta_quaternion_total_y = 0
delta_quaternion_total_z = 0
delta_quaternion_total_w = 0
File.readlines( 'output/delta_quaternion.txt' ).each do |line|
  values = line.split( ',' )
  quaternion_x = values[0].to_i
  quaternion_y = values[1].to_i
  quaternion_z = values[2].to_i
  quaternion_w = values[3].to_i
  delta_quaternion_x.push quaternion_x
  delta_quaternion_y.push quaternion_y
  delta_quaternion_z.push quaternion_z
  delta_quaternion_w.push quaternion_w
  delta_quaternion_total_x += quaternion_x
  delta_quaternion_total_y += quaternion_y
  delta_quaternion_total_z += quaternion_z
  delta_quaternion_total_w += quaternion_w
end
(0..10).each do |i|
  threshold = 2**i - 1
  count_x = 0
  count_y = 0
  count_z = 0
  count_w = 0
  0.upto(threshold) do |j|
    count_x += delta_quaternion_x[j]
    count_y += delta_quaternion_y[j]
    count_z += delta_quaternion_z[j]
    count_w += delta_quaternion_w[j]
  end
  percent_x = count_x / delta_quaternion_total_x.to_f * 100.0
  percent_y = count_y / delta_quaternion_total_y.to_f * 100.0
  percent_z = count_z / delta_quaternion_total_z.to_f * 100.0
  percent_w = count_w / delta_quaternion_total_z.to_f * 100.0
  puts "  #{threshold}: #{percent_x.round(1)}% #{percent_y.round(1)}% #{percent_z.round(1)}% #{percent_w.round(1)}%"
end

puts "\ndelta relative quaternion:\n\n"

delta_relative_quaternion_x = []
delta_relative_quaternion_y = []
delta_relative_quaternion_z = []
delta_relative_quaternion_w = []
delta_relative_quaternion_total_x = 0
delta_relative_quaternion_total_y = 0
delta_relative_quaternion_total_z = 0
delta_relative_quaternion_total_w = 0
File.readlines( 'output/delta_relative_quaternion.txt' ).each do |line|
  values = line.split( ',' )
  relative_quaternion_x = values[0].to_i
  relative_quaternion_y = values[1].to_i
  relative_quaternion_z = values[2].to_i
  relative_quaternion_w = values[3].to_i
  delta_relative_quaternion_x.push relative_quaternion_x
  delta_relative_quaternion_y.push relative_quaternion_y
  delta_relative_quaternion_z.push relative_quaternion_z
  delta_relative_quaternion_w.push relative_quaternion_w
  delta_relative_quaternion_total_x += relative_quaternion_x
  delta_relative_quaternion_total_y += relative_quaternion_y
  delta_relative_quaternion_total_z += relative_quaternion_z
  delta_relative_quaternion_total_w += relative_quaternion_w
end
(0..10).each do |i|
  threshold = 2**i - 1
  count_x = 0
  count_y = 0
  count_z = 0
  count_w = 0
  0.upto(threshold) do |j|
    count_x += delta_relative_quaternion_x[j]
    count_y += delta_relative_quaternion_y[j]
    count_z += delta_relative_quaternion_z[j]
    count_w += delta_relative_quaternion_w[j]
  end
  percent_x = count_x / delta_relative_quaternion_total_x.to_f * 100.0
  percent_y = count_y / delta_relative_quaternion_total_y.to_f * 100.0
  percent_z = count_z / delta_relative_quaternion_total_z.to_f * 100.0
  percent_w = count_w / delta_relative_quaternion_total_w.to_f * 100.0
  puts "  #{threshold}: #{percent_x.round(1)}% #{percent_y.round(1)}% #{percent_z.round(1)}% #{percent_w.round(1)}%"
end

puts "\nrelative quaternion values:\n"

class Quaternion
  attr_accessor :x, :y, :z, :w
  def initialize( x, y, z, w )
    @x = x
    @y = y
    @z = z
    @w = w
  end
end

RelativeQuaternionLargeW = 64
RelativeQuaternionSmallXYZ = 64
RelativeQuaternionReallySmallXYZ = 4    # 2  - 15.8%: 1 + 3 + 2 + 2 + 9 + 1 = 18 (saving of 12 bits)
RelativeQuaternionDeltaLimit = 256      # 4  - 19.0%: 1 + 3 + 3 + 3 + 9 + 1 = 20 (saving of 10 bits)
                                        # 8  - 22.6%: 1 + 3 + 4 + 4 + 9 + 1 = 22 (saving of 8 bits)
                                        # 16 - 26.9%: 1 + 3 + 5 + 5 + 9 + 1 = 24 (saving of 6 bits)

relative_quaternion_num_large_w = 0
relative_quaternion_small_xyz = 0
relative_quaternion_small_xyz_and_large_w = 0
relative_quaternion_two_or_more_really_small = 0

relative_quaternion_values = []

File.readlines( 'output/relative_quaternion_values.txt' ).each do |line|

  values = line.split( ',' )

  quaternion = Quaternion.new( values[0].to_i - 511, 
                               values[1].to_i - 511,
                               values[2].to_i - 511,
                               values[3].to_i - 511 )

  relative_quaternion_values.push quaternion
  
  if quaternion.x.abs >= RelativeQuaternionDeltaLimit || quaternion.y.abs >= RelativeQuaternionDeltaLimit || quaternion.z.abs >= RelativeQuaternionDeltaLimit
    next
  end

  if quaternion.w.abs >= RelativeQuaternionLargeW
    relative_quaternion_num_large_w += 1
  end

  if quaternion.x.abs < RelativeQuaternionSmallXYZ && quaternion.y.abs < RelativeQuaternionSmallXYZ && quaternion.z.abs < RelativeQuaternionSmallXYZ
    relative_quaternion_small_xyz += 1
  end
  
  num_really_small = 0

  if quaternion.x.abs < RelativeQuaternionReallySmallXYZ
    num_really_small += 1
  end

  if quaternion.y.abs < RelativeQuaternionReallySmallXYZ
    num_really_small += 1
  end

  if quaternion.z.abs < RelativeQuaternionReallySmallXYZ
    num_really_small += 1
  end

  if num_really_small >= 2 && quaternion.w.abs > RelativeQuaternionLargeW
    relative_quaternion_two_or_more_really_small += 1
  end

  if quaternion.x.abs < RelativeQuaternionSmallXYZ && quaternion.y.abs < RelativeQuaternionSmallXYZ && quaternion.z.abs < RelativeQuaternionSmallXYZ && quaternion.w.abs >= RelativeQuaternionLargeW
    relative_quaternion_small_xyz_and_large_w += 1
  end

#  puts "#{quaternion.x},#{quaternion.y},#{quaternion.z},#{quaternion.w}"

end

puts
puts "  + large w: #{((relative_quaternion_num_large_w/relative_quaternion_values.size.to_f)*100).round(1)}%"
puts "  + small xyz: #{((relative_quaternion_small_xyz/relative_quaternion_values.size.to_f)*100).round(1)}%"
puts "  + small xyz and large w: #{((relative_quaternion_small_xyz_and_large_w/relative_quaternion_values.size.to_f)*100).round(1)}%"
puts "  + two or more really small xyz: #{((relative_quaternion_two_or_more_really_small/relative_quaternion_values.size.to_f)*100).round(1)}%"
puts

puts "relative quaternion bandwidth estimate:\n\n"

def relative_quaternion_bandwidth_estimate quaternion, small_component_bits = 3, large_component_bits = 7
  num_really_small = 0
  small_threshold = ( 1 << ( small_component_bits - 1 ) ) - 1;
  if quaternion.x.abs < small_threshold
    num_really_small += 1
  end
  if quaternion.y.abs < small_threshold
    num_really_small += 1
  end
  if quaternion.z.abs < small_threshold
    num_really_small += 1
  end
  if num_really_small >= 2
    bits = 3 + small_component_bits*num_really_small + 1
    bits += 9 if num_really_small != 3
    return bits
  else
    return 2 + 9 + 9 + 9
  end  
end

absolute_quaternion_bits = 0
relative_quaternion_bits_1 = 0
relative_quaternion_bits_2 = 0
relative_quaternion_bits_3 = 0
relative_quaternion_bits_4 = 0
relative_quaternion_bits_5 = 0
relative_quaternion_bits_6 = 0
relative_quaternion_bits_7 = 0
relative_quaternion_bits_8 = 0
relative_quaternion_bits_9 = 0

relative_quaternion_values.each do |quaternion|
  absolute_quaternion_bits += 29
  relative_quaternion_bits_1 += relative_quaternion_bandwidth_estimate( quaternion, 1 )
  relative_quaternion_bits_2 += relative_quaternion_bandwidth_estimate( quaternion, 2 )
  relative_quaternion_bits_3 += relative_quaternion_bandwidth_estimate( quaternion, 3 )
  relative_quaternion_bits_4 += relative_quaternion_bandwidth_estimate( quaternion, 4 )
  relative_quaternion_bits_5 += relative_quaternion_bandwidth_estimate( quaternion, 5 )
  relative_quaternion_bits_6 += relative_quaternion_bandwidth_estimate( quaternion, 6 )
  relative_quaternion_bits_7 += relative_quaternion_bandwidth_estimate( quaternion, 7 )
  relative_quaternion_bits_8 += relative_quaternion_bandwidth_estimate( quaternion, 8 )
  relative_quaternion_bits_9 += relative_quaternion_bandwidth_estimate( quaternion, 9 )
end

puts "absolute quaternion bits: #{absolute_quaternion_bits}"
puts
puts "relative quaternion bits (2): #{relative_quaternion_bits_2} (#{(relative_quaternion_bits_2/absolute_quaternion_bits.to_f*100).round(1)}%)"
puts "relative quaternion bits (3): #{relative_quaternion_bits_3} (#{(relative_quaternion_bits_3/absolute_quaternion_bits.to_f*100).round(1)}%)"
puts "relative quaternion bits (4): #{relative_quaternion_bits_4} (#{(relative_quaternion_bits_4/absolute_quaternion_bits.to_f*100).round(1)}%)"
puts "relative quaternion bits (5): #{relative_quaternion_bits_5} (#{(relative_quaternion_bits_5/absolute_quaternion_bits.to_f*100).round(1)}%)"
puts "relative quaternion bits (6): #{relative_quaternion_bits_6} (#{(relative_quaternion_bits_6/absolute_quaternion_bits.to_f*100).round(1)}%)"
puts "relative quaternion bits (7): #{relative_quaternion_bits_7} (#{(relative_quaternion_bits_7/absolute_quaternion_bits.to_f*100).round(1)}%)"
puts "relative quaternion bits (8): #{relative_quaternion_bits_8} (#{(relative_quaternion_bits_8/absolute_quaternion_bits.to_f*100).round(1)}%)"
puts "relative quaternion bits (9): #{relative_quaternion_bits_9} (#{(relative_quaternion_bits_9/absolute_quaternion_bits.to_f*100).round(1)}%)"

puts

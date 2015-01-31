
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
  percent_w = count_w / delta_relative_quaternion_total_z.to_f * 100.0
  puts "  #{threshold}: #{percent_x.round(1)}% #{percent_y.round(1)}% #{percent_z.round(1)}% #{percent_w.round(1)}%"
end

puts

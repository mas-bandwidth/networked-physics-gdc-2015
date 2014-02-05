require 'Math'
require 'Sandbox'

class SimulationCubeState
  attr_accessor :position, :orientation, :linearVelocity, :angularVelocity, :enabled
  def initialize
    @position = Vector.new(0,0,0)
    @orientation = Quaternion.new(1,0,0,0)
    @linearVelocity = Vector.new(0,0,0)
    @angularVelocity = Vector.new(0,0,0)
    @enabled = true
  end
end

class SimulationInitialCubeState < SimulationCubeState
  attr_accessor :scale, :density
  def initialize
    @scale = 1.0
    @density = 1.0
    super
  end
end

def test_cube_add_remove
  
  simConfig = 
  { 
    :ERP => 0.001,
    :CFM => 0.001,
    :MaxIterations => 32,
    :MaximumCorrectingVelocity => 100.0,
    :ContactSurfaceLayer => 0.001,
    :Elasticity => 0.8,
    :Friction => 15.0,
    :Gravity => 20.0,
    :QuickStep => true,
    :MaxCubes => 32
  }

  simulation = Simulation.new simConfig

  simulation.addPlane Vector.new(0,0,1), 0

  puts '-------------------------------'
  cubes = Array.new
  5.times do
    initialState = SimulationInitialCubeState.new
    initialState.position = random_vector Vector.new(-10,-10,2), Vector.new(+10,+10,4)
    cubes.push simulation.addCube( initialState )
  end
  cubes.each { |id| puts "cube id = #{id}" }

  simulation.removeCube 0
  simulation.removeCube 1

  puts '-------------------------------'
  
  cubes.slice!(0,2)
  5.times { cubes.push simulation.addCube( nil ) }
  cubes.each { |id| puts "cube id = #{id}" }

  puts '-------------------------------'
  5.times do
    simulation.update 1.0 / 60.0
    cubeState = simulation.getCubeState 0
    puts "update"
  end

  puts '-------------------------------'
  simulation.reset
  cubes = Array.new
  5.times { cubes.push simulation.addCube( nil ) }
  cubes.each { |id| puts "cube id = #{id}" }

  puts '-------------------------------'
  Simulation.shutdown

end

def test_cube
  
  simConfig = 
  { 
    :ERP => 0.001,
    :CFM => 0.001,
    :MaxIterations => 32,
    :MaximumCorrectingVelocity => 100.0,
    :ContactSurfaceLayer => 0.001,
    :Elasticity => 0.8,
    :Friction => 15.0,
    :Gravity => 20.0,
    :QuickStep => true,
    :MaxCubes => 32
  }

  simulation = Simulation.new simConfig

  simulation.addPlane Vector.new(0,0,1), 0

  initialState = SimulationInitialCubeState.new
  initialState.position = Vector.new(0,0,10)
  simulation.addCube initialState

  40.times do
    simulation.update 1.0 / 60.0
    cubeState = simulation.getCubeState 0
    puts "position = #{cubeState.position}"
  end

  Simulation.shutdown

end

test_cube if __FILE__ == $0

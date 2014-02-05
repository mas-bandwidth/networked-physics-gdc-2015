# example game

require 'Scene'
require 'Simulation'

SendRate = 60
SimRate = 60
PacketSize = 2048
ServerPort = 40001
HibernationDistance = 5.0
HibernationPendingTime = 1.0
HibernationGreyArea = 0.0
MaxCubes = 128

SimulationConfig = 
{ 
  :ERP => 0.001,
  :CFM => 0.001,
  :MaxIterations => 64,
  :MaximumCorrectingVelocity => 10.0,
  :ContactSurfaceLayer => 0.001,
  :Elasticity => 0.6,
  :Friction => 15.0,
  :Gravity => 20.0,
  :QuickStep => true,
  :MaxCubes => MaxCubes
}

class Input
  attr_accessor :left, :right, :up, :down, :space
  def initialize
    @left = false
    @right = false
    @up = false
    @down = false
    @space = false
  end
end

class PhysicsComponent < Component
  RestTime = 0.2
  LinearRestThreshold = 0.05
  AngularRestThreshold = 0.05
  define_properties
  property :position,         { :type => DataType::Vector,      :default => Vector.new(0,0,0) }
  property :orientation,      { :type => DataType::Quaternion,  :default => Quaternion.new(1,0,0,0) }
  property :linearVelocity,   { :type => DataType::Vector,      :default => Vector.new(0,0,0) }
  property :angularVelocity,  { :type => DataType::Vector,      :default => Vector.new(0,0,0) }
  property :enabled,          { :type => DataType::Boolean,     :default => true }
  property :scale,            { :type => DataType::Float,       :default => 1.0,     :constant => true }
  property :density,          { :type => DataType::Float,       :default => 1.0,     :constant => true }
  attr_reader :cubeId, :timeAtRest
  def create
    # ...
  end
  def unhibernate
    initialState = SimulationInitialCubeState.new
    initialState.position = position
    initialState.orientation = orientation
    initialState.linearVelocity = linearVelocity
    initialState.angularVelocity = angularVelocity
    initialState.enabled = enabled
    initialState.scale = scale
    initialState.density = density
    @cubeId = $simulation.addCube initialState
    @timeAtRest = @enabled ? 0.0 : RestTime
    raise 'too many cubes' if @cubeId == -1
    puts "unhibernate cube #{@cubeId}"
  end
  def hibernate
    puts "hibernate cube #{@cubeId}"
    $simulation.removeCube @cubeId
    @cubeId = 0
  end
  def destroy
    # ...
  end
  def update deltaTime
    if @linearVelocity.length < LinearRestThreshold && @angularVelocity.length < AngularRestThreshold
      @timeAtRest += deltaTime 
    else
      @timeAtRest = 0.0
    end
    @enabled = @timeAtRest < RestTime
  end
end

class PropComponent < Component
  define_properties
  def update deltaTime
    # ...
  end
end

class HumanComponent < Component
  define_properties
  def update deltaTime
    # ...
  end
end

class PickupComponent < Component
  define_properties
end

class BulletComponent < Component
  define_properties
  property :origin,     { :type => DataType::Vector,      :default => Vector.new(0,0,0), :constant => true }
  property :direction,  { :type => DataType::UnitVector,  :default => Vector.new(0,0,1), :constant => true }
  property :time,       { :type => DataType::Float,       :default => 0.0 }
end

class GrenadeComponent < Component
  define_properties
  property :time,             { :type => DataType::Float,  :default => 0.0 }
  property :explodeTime,      { :type => DataType::Float,  :default => 1.0, :constant => true }
  property :initialVelocity,  { :type => DataType::Vector, :default => Vector.new(0,0,0), :constant => true }
end

class ExplosionComponent < Component
  define_properties
  property :time,             { :type => DataType::Float,   :default => 0.0 }
  property :startTime,        { :type => DataType::Float,   :default => 0.0, :constant => true }
  property :finishTime,       { :type => DataType::Float,   :default => 1.0, :constant => true }
  property :initialRadius,    { :type => DataType::Float,   :default => 0.0, :constant => true }
  property :maximumRadius,    { :type => DataType::Float,   :default => 10.0, :constant => true }
end

class HealthComponent < Component
  define_properties
  property :currentHealth, { :type => DataType::Float, :default => 100.0, :minimum => 0.0, :maximum => 100.0, :resolution => 1.0 }
  property :maximumHealth, { :type => DataType::Float, :default => 100.0, :constant => true }
end

# schema object schema

class GameObjectSchema < ComponentObjectSchema
  def declare_component_map
    {
      :physics => PhysicsComponent,
      :human => HumanComponent,
      :prop => PropComponent,
      :pickup => PickupComponent,
      :bullet => BulletComponent,
      :grenade => GrenadeComponent,
      :explosion => ExplosionComponent,
      :health => HealthComponent,
    }
  end
  def declare_object_components
    {
      :cube => [ :physics, :prop, :health ],
    }
  end
end

def test_game
  
  # setup object builder
  
  schema = GameObjectSchema.new
  objectBuilder = ComponentObjectBuilder.new schema
  
  # create cubes scattered about
  
  objects = Hash.new
  id = 1
  MaxCubes.times do
    object = objectBuilder.buildObject id, :cube
    physicsComponent = object.components[0]
    physicsComponent.position = random_vector Vector.new( -10, -10, 2 ), Vector.new( +10, +10, 10 )
    physicsComponent.scale = random_float 0.1, 1.0
    objects[id] = object
    id += 1
  end

  # save out to game.xml to verify

  objectBuilder.save 'game.xml', objects
  
  # create object database

  objectDatabase = ComponentObjectDatabase.new schema, objectBuilder, 'objects', 'component'
  objectDatabase.connect 'localhost', 'test', 'testuser', 'testpass'
  objectDatabase.clear
  objectDatabase.create

  # create hibernation system and import objects

  hibernationSystem = HibernationSystem.new objectBuilder, objectDatabase, HibernationPendingTime
  hibernationSystem.import objects

  # create relevancy system and hook up to hibernation
  
  relevancySystem = RelevancySystem.new HibernationDistance, HibernationGreyArea
  objects.each_pair do |id,object|
    position = object.components[0].position
    objectEntry = RelevancyObjectEntry.new( id, position )
    relevancySystem.addObject objectEntry
  end
  relevancySystem.listeners.push hibernationSystem
  objects = nil

  # setup game server
  
  Net::initialize
  
  connection = Connection.new
  puts "server listening on port #{ServerPort}"
  raise 'failed to start server' if not connection.listen ServerPort

  # create simulation

  $simulation = Simulation.new SimulationConfig

  $simulation.addPlane Vector.new(0,0,1), 0

  time = 0.0
  deltaTime = 1.0 / SimRate
  sendAccumulator = 0.0

  # main loop

  input = Input.new

  clientConnected = false

  while true

    # detect client connect, disconnect
    
    if !clientConnected and connection.connected?
      puts 'client connected'
      clientConnected = true
    elsif clientConnected and !connection.connected?
      puts 'client disconnected'
      clientConnected = false
    end    
    
    # update relevancy and hibernation systems

    hibernationSystem.runtimeObjects.each_pair do |id,object|
      physicsComponent = object.components[0]
      relevancySystem.updateObjectPosition id, physicsComponent.position
    end

    relevancySystem.referencePoints[0].active = connection.connected?
    
    relevancySystem.update

    hibernationSystem.update deltaTime
    
	  # send packets @ send rate

    if connection.connected?
      
      sendAccumulator += deltaTime

		  if sendAccumulator >= ( 1.0 / SendRate )

  			packet = Packet.new( PacketSize )
		
  			packet.beginWrite
  			
  			origin = relevancySystem.referencePoints[0].position

        packet.writeFloat origin.x
        packet.writeFloat origin.y
        packet.writeFloat origin.z

  		  packet.writeInteger hibernationSystem.runtimeObjects.size

        hibernationSystem.runtimeObjects.each_pair do |id,object|
          physicsComponent = object.components[0]
          packet.writeInteger id
          packet.writeFloat physicsComponent.position.x
          packet.writeFloat physicsComponent.position.y
          packet.writeFloat physicsComponent.position.z
          packet.writeFloat physicsComponent.orientation.w
          packet.writeFloat physicsComponent.orientation.x
          packet.writeFloat physicsComponent.orientation.y
          packet.writeFloat physicsComponent.orientation.z
          packet.writeFloat physicsComponent.scale
          pendingHibernation = hibernationSystem.pendingHibernation? id
          packet.writeBoolean pendingHibernation
        end
		
  			raise 'failed to send packet' if not connection.send packet, time

        sendAccumulator = 0.0
	  
  	  end

  	else
  	  
  	  sendAccumulator = 0.0
  	  
  	end
  	
  	# receive packets

		while true

		  packet = Packet.new( PacketSize )

      break if not connection.receive packet, time

      # todo: handle out of sequence packets!

      input.left = packet.readBoolean
      input.right = packet.readBoolean
      input.up = packet.readBoolean
      input.down = packet.readBoolean
      input.space = packet.readBoolean
      
		end

    connection.update time, deltaTime

    # process input
    
    if input.space
      hibernationSystem.runtimeObjects.each_pair do |id,object|
        physicsComponent = object.components[0]
        physicsComponent.linearVelocity = Vector.new(0,0,10)
      end
    end

    speed = 4.0 * deltaTime
    movement = Vector.new(0,0,0)
    movement.x -= speed if input.left
    movement.x += speed if input.right
    movement.y -= speed if input.down
    movement.y += speed if input.up
    relevancySystem.referencePoints[0].position += movement

    # update simulation

    hibernationSystem.runtimeObjects.each_pair do |id,object|
      physicsComponent = object.components[0]
      cubeState = SimulationCubeState.new
      cubeState.position = physicsComponent.position
      cubeState.orientation = physicsComponent.orientation
      cubeState.linearVelocity = physicsComponent.linearVelocity
      cubeState.angularVelocity = physicsComponent.angularVelocity
      cubeState.enabled = physicsComponent.enabled
      $simulation.setCubeState physicsComponent.cubeId, cubeState
    end

    $simulation.update deltaTime
    
    hibernationSystem.runtimeObjects.each_pair do |id,object|
      physicsComponent = object.components[0]
      cubeState = $simulation.getCubeState physicsComponent.cubeId
      physicsComponent.position = cubeState.position
      physicsComponent.orientation = cubeState.orientation
      physicsComponent.linearVelocity = cubeState.linearVelocity
      physicsComponent.angularVelocity = cubeState.angularVelocity
    end

    # update game objects
    
    hibernationSystem.runtimeObjects.each_pair do |id,object|
      object.update deltaTime
    end
        
    # wait for frame
 
 # todo: need to implement wait for frame again!
        
#    Platform::waitForFrame deltaTime
    
    time += deltaTime

  end

  Simulation::shutdown
  Net::shutdown
  
end

test_game if __FILE__ == $0

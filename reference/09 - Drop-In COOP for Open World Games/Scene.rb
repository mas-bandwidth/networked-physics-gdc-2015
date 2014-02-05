# scene 
#  - extends from abstract component objects to scene object with position, relevancy and hibernation system concepts

require 'Components'

class RelevancyObjectEntry
  attr_reader :id
  attr_accessor :position, :relevancy
  def initialize id, position
    @id = id
    @position = position
    @relevancy = 0
  end
end

class ReferencePoint
  attr_accessor :active, :position, :relevancy
  def initialize
    @active = false
    @position = Vector.new(0,0,0)
  end
end

class RelevancyListener
  def onRelevancyChange id, oldRelevancy, newRelevancy
    puts "relevancy change: #{id}: #{oldRelevancy} -> #{newRelevancy}"
  end
end

class RelevancySystem
  attr_reader :referencePoints, :objects
  attr_accessor :listeners, :relevantDistance, :relevantGreyArea
  def initialize relevantDistance = 10.0, relevantGreyArea = 1.0, maximumReferencePoints = 4
    raise 'too many reference points' if maximumReferencePoints > 32
    @objects = Hash.new
    @listeners = Array.new
    @referencePoints = Array.new
    @relevantDistance = relevantDistance
    @relevantGreyArea = relevantGreyArea
    maximumReferencePoints.times { @referencePoints.push ReferencePoint.new }
  end
  def addObject objectEntry
    @objects[objectEntry.id] = objectEntry
  end
  def removeObject id
    @objects.remove id
  end
  def updateObjectPosition id, position
    objectEntry = @objects[id]
    raise "no relevancy entry for object id #{id}" if objectEntry == nil
    objectEntry.position = position
  end
  def update
    # note: this is an *exceptionally* slow algorithm that needs to be rewritten, perhaps with a quadtree based representation
    @objects.each do |id,object|
      oldRelevancy = object.relevancy
      newRelevancy = 0
      @referencePoints.each_index do |index|
        referencePoint = @referencePoints[index]
        next if not referencePoint.active
        distance = @relevantDistance
        distance += @relevantGreyArea if oldRelevancy != 0
        if ( object.position - referencePoint.position ).length2d < distance
          newRelevancy |= ( 1 << index )
        end
      end
      @listeners.each { |listener| listener.onRelevancyChange( object.id, oldRelevancy, newRelevancy ) if oldRelevancy != newRelevancy }
      object.relevancy = newRelevancy
    end
  end
end

def test_relevancy_system
  puts '--------------------------------------------------------------------'
  relevantDistance = 1.0
  relevancySystem = RelevancySystem.new relevantDistance
  puts 'reference points:'
  index = 0
  relevancySystem.referencePoints.each do |referencePoint|
    puts " #{index}: active = #{referencePoint.active}, position = #{referencePoint.position}"
    index += 1
  end
  puts 'object entries:'
  10.times { |index| relevancySystem.addObject RelevancyObjectEntry.new( index + 1, random_vector( Vector.new(-2,0,-2), Vector.new(2,0,2) ) ) }
  relevancySystem.objects.each do |objectEntry|
    puts " #{objectEntry.id}: position = #{objectEntry.position}"
  end
  puts '--------------------------------------------------------------------'
  relevancySystem.listeners.push RelevancyListener.new
  relevancySystem.referencePoints[0].active = true
  10.times { relevancySystem.update }
  puts '--------------------------------------------------------------------'
  relevancySystem.referencePoints[0].active = false
  10.times { relevancySystem.update }
  puts '--------------------------------------------------------------------'
end

class HibernationListener
  def onHibernateObject id
    puts "hibernate object #{id}"
  end
  def onUnhibernateObject id
    puts "unhibernate object #{id}"
  end
  def onHibernatePending id
    puts "hibernate pending #{id}"
  end
  def onHibernatePendingAbort id
    puts "hibernate pending abort #{id}"
  end
end

class PendingHibernationEntry
  attr_accessor :time
  def initialize
    @time = 0.0
  end
end

class HibernationSystem
  attr_accessor :listeners
  def initialize objectBuilder, componentDatabase, pendingHibernationTime = 1.0
    @objectBuilder = objectBuilder
    @componentDatabase = componentDatabase
    @runtimeObjects = Hash.new
    @listeners = Array.new
    @pendingHibernation = Hash.new
    @pendingHibernationTime = pendingHibernationTime
  end
  def import objects
    objects.each_pair do |id,object|
      @componentDatabase.storeObject id, object
    end
  end
  def unhibernate id
    object = @componentDatabase.retrieveObject id
    raise "cannot unhibernate object id #{id}" if object == nil
    @runtimeObjects[id] = object    
    @listeners.each { |listener| listener.onUnhibernateObject id }
    object.unhibernate
  end
  def hibernate id
    object = @runtimeObjects[id]
    raise "cannot hibernate object id #{id}" if object == nil
    object.hibernate
    @listeners.each { |listener| listener.onHibernateObject id }
    @componentDatabase.storeObject id, object
    @runtimeObjects.delete id
  end
  def hibernated? id
    @runtimeObjects[id] == nil
  end
  def unhibernated? id
    @runtimeObjects[id] != nil
  end
  def pendingHibernation? id
    @pendingHibernation[id] != nil
  end
  def runtimeObjects
    @runtimeObjects
  end
  def onRelevancyChange id, oldRelevancy, newRelevancy
    if oldRelevancy == 0 and newRelevancy != 0
      if @pendingHibernation[id] != nil
        @pendingHibernation.delete id
        @listeners.each { |listener| listener.onHibernatePendingAbort id }
      else
        unhibernate id
      end
    elsif oldRelevancy != 0 and newRelevancy == 0
      raise 'already pending hibernation' if @pendingHibernation[id]
      @pendingHibernation[id] = PendingHibernationEntry.new
      @listeners.each { |listener| listener.onHibernatePending id }
    end
  end
  def update deltaTime
    @pendingHibernation.delete_if do |id,entry|
      raise 'pending hibernation without runtime' if @runtimeObjects[id] == nil
      entry.time += deltaTime
      if entry.time >= @pendingHibernationTime
        hibernate id if entry.time >= @pendingHibernationTime
        true
      else
        false
      end
    end
  end
end

def test_hibernation_system
  schema = ComponentObjectSchema.new
  objectBuilder = ComponentObjectBuilder.new schema
  objectDatabase = ComponentObjectDatabase.new schema, objectBuilder, 'objects', 'component'
  objectDatabase.connect 'localhost', 'test', 'testuser', 'testpass'
  objectDatabase.clear
  objectDatabase.create
  objects = objectBuilder.load 'scene.xml'
  hibernationSystem = HibernationSystem.new objectBuilder, objectDatabase
  hibernationSystem.import objects
  hibernationSystem.unhibernate 1
  hibernationSystem.unhibernate 2
  runtimeObjects = hibernationSystem.runtimeObjects
  runtimeObjects.each_pair do |id,object|
    puts '------------------------------------------------------------------------'
    puts "[object #{id}]"
    object.components.each { |component| component.print }
  end
  puts '------------------------------------------------------------------------'
  hibernationSystem.hibernate 1
  hibernationSystem.hibernate 2
  runtimeObjects = hibernationSystem.runtimeObjects
  raise 'failed to hibernate objects' if runtimeObjects.size != 0
end

#test_hibernation_system if __FILE__ == $0

class PhysicsComponent < Component
  define_properties
  property :position,             { :type => DataType::Vector,      :default => Vector.new(0,0,0), :minimum => Vector.new(-100,-100,-100), :maximum => Vector.new(+100,+100,+100) }
  property :orientation,          { :type => DataType::Quaternion,  :default => Quaternion.new(10,0,0,0) }
end

class SceneObjectSchema < ComponentObjectSchema
  def declare_component_map
    {
      :physics => PhysicsComponent,
    }
  end
  def declare_object_components
    {
      :SceneObject => [ :physics ],
    }
  end
end


def test_hibernation_with_relevancy
  # create scene objects
  schema = SceneObjectSchema.new
  objectBuilder = ComponentObjectBuilder.new schema
  objects = Hash.new
  10.times do |index|
    id = index + 1
    object = objectBuilder.buildObject id, :SceneObject
    object.components[0].position = random_vector( Vector.new(-2,0,-2), Vector.new(2,0,2) )
    objects[id] = object
  end
  # create object database
  objectDatabase = ComponentObjectDatabase.new schema, objectBuilder, 'objects', 'component'
  objectDatabase.connect 'localhost', 'test', 'testuser', 'testpass'
  objectDatabase.clear
  objectDatabase.create
  # create hibernation system and import objects
  hibernationSystem = HibernationSystem.new objectBuilder, objectDatabase
  hibernationSystem.import objects
  hibernationSystem.listeners.push HibernationListener.new
  # create relevancy system and mess around with reference points
  puts '--------------------------------------------------------------------'
  relevantDistance = 1.0
  relevancySystem = RelevancySystem.new relevantDistance
  objectIds = objects.keys.sort
  objectIds.each do |id|
    object = objects[id]
    position = object.components[0].position
    objectEntry = RelevancyObjectEntry.new( id, position )
    puts " #{objectEntry.id}: position = #{objectEntry.position}"
    relevancySystem.addObject objectEntry
  end
  puts '--------------------------------------------------------------------'
  relevancySystem.listeners.push hibernationSystem
  relevancySystem.referencePoints[0].active = true
  11.times { relevancySystem.update; hibernationSystem.update 0.1 }
  puts '--------------------------------------------------------------------'
  relevancySystem.referencePoints[0].active = false
  11.times { relevancySystem.update; hibernationSystem.update 0.1 }
  puts '--------------------------------------------------------------------'
end

#test_hibernation_with_relevancy if __FILE__ == $0

def test_pending_hibernation
  # create scene objects
  schema = SceneObjectSchema.new
  objectBuilder = ComponentObjectBuilder.new schema
  objects = Hash.new
  10.times do |index|
    id = index + 1
    object = objectBuilder.buildObject id, :SceneObject
    object.components[0].position = random_vector( Vector.new(-2,0,-2), Vector.new(2,0,2) )
    objects[id] = object
  end
  # create object database
  objectDatabase = ComponentObjectDatabase.new schema, objectBuilder, 'objects', 'component'
  objectDatabase.connect 'localhost', 'test', 'testuser', 'testpass'
  objectDatabase.clear
  objectDatabase.create
  # create hibernation system and import objects
  hibernationSystem = HibernationSystem.new objectBuilder, objectDatabase
  hibernationSystem.import objects
  hibernationSystem.listeners.push HibernationListener.new
  # unhibernate some objects
  puts '--------------------------------------------------'
  hibernationSystem.onRelevancyChange 1, 0, 1
  hibernationSystem.onRelevancyChange 2, 0, 1
  hibernationSystem.onRelevancyChange 3, 0, 1
  # hibernate them
  hibernationSystem.onRelevancyChange 1, 1, 0
  hibernationSystem.onRelevancyChange 2, 1, 0
  hibernationSystem.onRelevancyChange 3, 1, 0
  # update for one second until they hibernate
  puts '--------------------------------------------------'
  11.times { puts 'tick'; hibernationSystem.update 0.1 }
  puts '--------------------------------------------------'
end

def test_pending_hibernation_with_relevancy
  # create scene objects
  schema = SceneObjectSchema.new
  objectBuilder = ComponentObjectBuilder.new schema
  objects = Hash.new
  10.times do |index|
    id = index + 1
    object = objectBuilder.buildObject id, :SceneObject
    object.components[0].position = random_vector( Vector.new(-0.1,-0.1,0), Vector.new(+0.1,+0.1,0) )
    objects[id] = object
  end
  # create object database
  objectDatabase = ComponentObjectDatabase.new schema, objectBuilder, 'objects', 'component'
  objectDatabase.connect 'localhost', 'test', 'testuser', 'testpass'
  objectDatabase.clear
  objectDatabase.create
  # create hibernation system and import objects
  hibernationSystem = HibernationSystem.new objectBuilder, objectDatabase
  hibernationSystem.import objects
  hibernationSystem.listeners.push HibernationListener.new
  # create relevancy system and import objcets
  puts '--------------------------------------------------------------------'
  relevantDistance = 1.0
  relevancySystem = RelevancySystem.new relevantDistance
  objectIds = objects.keys.sort
  objectIds.each do |id|
    object = objects[id]
    position = object.components[0].position
    objectEntry = RelevancyObjectEntry.new( id, position )
    puts " #{objectEntry.id}: position = #{objectEntry.position}"
    relevancySystem.addObject objectEntry
  end
  # update a few times and unhibernate some objects
  puts '--------------------------------------------------------------------'
  relevancySystem.listeners.push hibernationSystem
  relevancySystem.referencePoints[0].active = true
  11.times { relevancySystem.update; hibernationSystem.update 0.1 }
  # move reference point to the side and watch what objects unhibernate
  1000.times do
    relevancySystem.referencePoints[0].position.x += 0.01
    relevancySystem.update
    hibernationSystem.update 0.1
  end
  puts '--------------------------------------------------------------------'
end

test_pending_hibernation_with_relevancy if __FILE__ == $0

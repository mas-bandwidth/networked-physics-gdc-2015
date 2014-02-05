require 'mysql'

begin
  
  mysql = Mysql.new( 'localhost', 'testuser', 'testpass', 'test' )

  mysql.query "DROP TABLE IF EXISTS test_component"
  
  mysql.query "CREATE TABLE test_component
             (
               id                     INT,
               booleanProperty        ENUM('true','false'),
               integerProperty        INT,
               floatProperty          FLOAT,
               vectorProperty_x       FLOAT,
               vectorProperty_y       FLOAT,
               vectorProperty_z       FLOAT,
               quaternionProperty_w   FLOAT,
               quaternionProperty_x   FLOAT,
               quaternionProperty_y   FLOAT,
               quaternionProperty_z   FLOAT
             )
             "
  mysql.query "INSERT INTO test_component ( id, booleanProperty, integerProperty, floatProperty, vectorProperty_x, vectorProperty_y, vectorProperty_z, quaternionProperty_w, quaternionProperty_x, quaternionProperty_y, quaternionProperty_z )
               VALUES
                 ( 1, 'true', 10, 1.25, 1,2,3, 1,0,0,0  ),
                 ( 2, 'false', 15, 0.333, 0,0,1, 0,1,0,0 )
             "
             
  puts "Number of rows inserted: #{mysql.affected_rows}"
  
  results = mysql.query "SELECT * FROM test_component WHERE id = 1"

  results.each do |row|
    puts row.join(',')
  end
  
  puts "Number of rows returned: #{results.num_rows}" 
  
  results.free  

rescue Mysql::Error => e
  
  puts "Error code: #{e.errno}"
  puts "Error message: #{e.error}"
  puts "Error SQLSTATE: #{e.sqlstate}" if e.respond_to?("sqlstate")

ensure
  
  mysql.close if mysql

end

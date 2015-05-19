<?php
include("connect.php");

try {
    $conn = new PDO("mysql:host=$servername;dbname=$dbname", $username, $password);
    $conn->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

	$starting = 0;
	$requested  = (int)intval($_POST["starting_record"]);
	if ($requested > $starting){
		$starting = $requested;
	}
	$ending = $starting + 100;
    $stmt = $conn->prepare("SELECT * FROM req_color ORDER BY id DESC LIMIT :starting,:ending");
	$stmt->bindParam(':starting', $starting, PDO::PARAM_INT);
	$stmt->bindParam(':ending', $ending, PDO::PARAM_INT);
	
	$stmt->execute();
	$rowarray = $stmt->fetchAll();
 
print "<table>\n";  

foreach ($rowarray as $row) {
	print "<tr>";  
        print "<td>" .  $row["CreatedTime"]  . "</td><td bgcolor=\"" . $row["color1"] . "\">" . $row["color1"] . "</td>". "<td bgcolor=\"" . $row["color2"] . "\">" .$row["color2"] . "</td>";	
    print "</tr>\n";
}  
print "</table>\n";  
}
catch (PDOException $e) {
    print "Error: " . $e->getMessage();
}
$conn = null;

?>


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
    $stmt = $conn->prepare("SELECT * FROM req_msg ORDER BY id DESC LIMIT :starting,:ending");
	$stmt->bindParam(':starting', $starting, PDO::PARAM_INT);
	$stmt->bindParam(':ending', $ending, PDO::PARAM_INT);
	
	$stmt->execute();
	$rowarray = $stmt->fetchAll();
 
print "<table>\n";  

foreach ($rowarray as $row) {
	
	#add a space every 15 characters formatted to fit on a phone
	#this is just an estimation, some characters are wider than others...
	
	$output_string = "";
	$char_counter = 0;
	for ($i=0; $i<strlen($row["user_msg"]); $i++) {
		if (strlen(trim($row["user_msg"][$i])) == 0){
			$char_counter = 0;
		}else{
			$char_counter++;
		}
		if ($char_counter > 14){
		$output_string = $output_string . " ";	
			$char_counter = 0;
		}
		$output_string = $output_string . $row["user_msg"][$i];
	}
	
	print "<tr>\n";  
        print "<td>"  . $row["CreatedTime"] . "</td><td>" . $output_string . "</td>";
    print "</tr>\n";
}  

#spacer for date formatting
print "<td><font color=\"black\">0000000000</font></td><td></td>";
print "</table>\n";  
}
catch (PDOException $e) {
    print "Error: " . $e->getMessage();
}
$conn = null;

?>
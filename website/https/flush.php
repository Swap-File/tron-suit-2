<?php
include("connect.php");

try {
    $conn = new PDO("mysql:host=$servername;dbname=$dbname", $username, $password);
    $conn->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
            
	$stmt = $conn->prepare("UPDATE req_color SET sent=TRUE WHERE sent=FALSE");
	$stmt->execute();
	
	$stmt = $conn->prepare("UPDATE req_msg SET sent=TRUE WHERE sent=FALSE");
	$stmt->execute();
	
	echo "Flush Complete";
}
catch (PDOException $e) {
    print "Error: " . $e->getMessage();
}
$conn = null;

?>
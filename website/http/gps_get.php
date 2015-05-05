<?php
include("connect.php");

try {
    $conn = new PDO("mysql:host=$servername;dbname=$dbname", $username, $password);
    $conn->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
        
    $stmt = $conn->prepare("SELECT * FROM log ORDER BY id DESC LIMIT 1");
    $stmt->execute();
    $result = $stmt->fetchAll();
    
    if (count($result) > 0) {
		$row = $result[0];
        echo (1000*strtotime($row["timeStamp"]. ' GMT')) ."\t" . 
		$row["latitude"]. "\t" . $row["longitude"]."\t" . $row["accuracy"] ."\n";

    }
}
catch (PDOException $e) {
    print "Error: " . $e->getMessage();
}
$conn = null;

?>
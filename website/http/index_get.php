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
        echo strtotime($row["timeStamp"]. ' GMT') ."\t" . 
		$row["color1"]. "\t" . $row["color2"]."\t";
    }
}
catch (PDOException $e) {
    print "Error: " . $e->getMessage();
}
$conn = null;

?>
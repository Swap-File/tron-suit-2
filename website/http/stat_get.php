<?php
include("connect.php");

try {
    $conn = new PDO("mysql:host=$servername;dbname=$dbname", $username, $password);
    $conn->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
        
    $stmt = $conn->prepare("SELECT * FROM log ORDER BY id DESC LIMIT 1");
    $stmt->execute();
    $result = $stmt->fetchAll();
    
    foreach ($result as $row) {
        echo strtotime($row["timeStamp"]. ' GMT') ."\t" . 
		$row["gloveppsout"]. "\t" . $row["glove0ppsin"]."\t" . $row["glove1ppsin"]."\t" . $row["glovelost"]."\t" . 
		$row["disc0ppsout"]. "\t" . $row["disc0ppsin"]."\t" . $row["disc0lost"] ."\n";
    }
}

catch (PDOException $e) {
    print "Error: " . $e->getMessage();
}
$conn = null;

?>
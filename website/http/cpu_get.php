<?php
include("connect.php");

try {
    $conn = new PDO("mysql:host=$servername;dbname=$dbname", $username, $password);
    $conn->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
        
    $stmt = $conn->prepare("SELECT * FROM log ORDER BY id DESC LIMIT 1");
    $stmt->execute();
    $result = $stmt->fetchAll();
    
    foreach ($result as $row) {
        echo (1000 *strtotime($row["timeStamp"]. ' GMT')) ."\t" . 
		$row["glove0cpu"]. "\t" . $row["glove1cpu"]."\t" . $row["disc0cpu"]."\t" . $row["suit0cpu"]."\t" ."\n";

    }
}
catch (PDOException $e) {
    print "Error: " . $e->getMessage();
}
$conn = null;

?>
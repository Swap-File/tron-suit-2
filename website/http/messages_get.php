<?php
include("connect.php");

try {
    $conn = new PDO("mysql:host=$servername;dbname=$dbname", $username, $password);
    $conn->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
    
    $stmt = $conn->prepare("SELECT * FROM messages ORDER BY id DESC LIMIT 1");
    $stmt->execute();
    $result = $stmt->fetchAll();
    
    foreach ($result as $row) {
        echo date('m-d-Y \a\t h:i:sa', strtotime($row["CreatedTime"])) . "\t" .
		$row["user_msg"] . "\t" .
		$row["color1"] . "\t" .
		$row["color2"] . "\t" .
		$row["ip"] . "\n";
    }
}
catch (PDOException $e) {
    print "Error: " . $e->getMessage();
}
$conn = null;

?>
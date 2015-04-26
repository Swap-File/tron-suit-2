<?php
include("connect.php");

try {
    $conn = new PDO("mysql:host=$servername;dbname=$dbname", $username, $password);
    $conn->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
        
    $stmt = $conn->prepare("SELECT * FROM log ORDER BY timeStamp DESC LIMIT 1");
    $stmt->execute();
    $result = $stmt->fetchAll();
    
    foreach ($result as $row) {
        echo strtotime($row["timeStamp"]. ' GMT') ."\t" . $row["number1"].  "\t" . $row["number2"]. "\n";
    }
}
catch (PDOException $e) {
    print "Error: " . $e->getMessage();
}
$conn = null;

?>
<?php
include("connect.php");

try {
    $conn = new PDO("mysql:host=$servername;dbname=$dbname", $username, $password);
    $conn->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
    
    $stmt = $conn->prepare("INSERT INTO log (number1, number2) 
    VALUES (:number1, :number2)");
    
    $stmt->bindParam(':number1', $_POST["number1"], PDO::PARAM_INT);
    $stmt->bindParam(':number2', $_POST["number2"], PDO::PARAM_INT);
    $stmt->execute();
    
    $stmt = $conn->prepare("SELECT * FROM messages WHERE sent IS FALSE ORDER BY id ASC LIMIT 1");
    $stmt->execute();
    $result = $stmt->fetchAll();
    
    if (count($result) > 0) {
        $row = $result[0];
        echo date('m-d-Y \a\t h:i:sa', strtotime($row["CreatedTime"])) . "\t" . $row["user_msg"] . "\t" . $row["color1_r"] . "\t" . $row["color1_g"] . "\t" . $row["color1_b"] . "\t" . $row["color2_r"] . "\t" . $row["color2_g"] . "\t" . $row["color2_b"] . "\t" . $row["ip"] . "\n";
        
        $stmt = $conn->prepare("UPDATE messages SET sent=TRUE WHERE id=(:id)");
        $stmt->bindParam(':id', $row["id"], PDO::PARAM_INT);
        $stmt->execute();
    }
}
catch (PDOException $e) {
    print "Error: " . $e->getMessage();
}
$conn = null;

?>
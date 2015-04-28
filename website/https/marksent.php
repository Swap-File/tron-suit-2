<?php
include("connect.php");

try {
    $conn = new PDO("mysql:host=$servername;dbname=$dbname", $username, $password);
    $conn->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
    
    $stmt = $conn->prepare("UPDATE messages SET sent=TRUE WHERE sent=FALSE");
    $stmt->bindParam(':id', $row["id"], PDO::PARAM_INT);
    $stmt->execute();
}
catch (PDOException $e) {
    print "Error: " . $e->getMessage();
}
$conn = null;

?>
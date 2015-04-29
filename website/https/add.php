<?php
include("connect.php");

try {
    $conn = new PDO("mysql:host=$servername;dbname=$dbname", $username, $password);
    $conn->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
    
    $stmt = $conn->prepare("INSERT INTO log (color1,color2,glove0cpu,glove1cpu,disc0cpu,suit0cpu,gloveppsout, glove0ppsin,glove1ppsin,glovelost,disc0ppsout,disc0ppsin,disc0lost,	glove0yaw,glove0pitch,glove0roll,glove1yaw,glove1pitch,glove1roll,latitude,longitude,accuracy,ip) 
    VALUES (:color1,:color2,:glove0cpu,:glove1cpu,:disc0cpu,:suit0cpu,:gloveppsout, :glove0ppsin,:glove1ppsin,:glovelost,:disc0ppsout,:disc0ppsin,:disc0lost,:glove0yaw,:glove0pitch,:glove0roll,:glove1yaw,:glove1pitch,:glove1roll,:latitude,:longitude,:accuracy,:ip)");
    	
    $stmt->bindParam(':color1', strtoupper($_POST["color1"]), PDO::PARAM_STR);
    $stmt->bindParam(':color2', strtoupper($_POST["color2"]), PDO::PARAM_STR);

	   
	$stmt->bindParam(':glove0cpu', $_POST["glove0cpu"], PDO::PARAM_INT);
    $stmt->bindParam(':glove1cpu', $_POST["glove1cpu"], PDO::PARAM_INT);
	$stmt->bindParam(':disc0cpu', $_POST["disc0cpu"], PDO::PARAM_INT);
    $stmt->bindParam(':suit0cpu', $_POST["suit0cpu"], PDO::PARAM_INT);
	
	$stmt->bindParam(':gloveppsout', $_POST["gloveppsout"], PDO::PARAM_INT);
    $stmt->bindParam(':glove0ppsin', $_POST["glove0ppsin"], PDO::PARAM_INT);
	$stmt->bindParam(':glove1ppsin', $_POST["glove1ppsin"], PDO::PARAM_INT);
    $stmt->bindParam(':glovelost', $_POST["glovelost"], PDO::PARAM_INT);

    $stmt->bindParam(':disc0ppsout', $_POST["disc0ppsout"], PDO::PARAM_INT);
	$stmt->bindParam(':disc0ppsin', $_POST["disc0ppsin"], PDO::PARAM_INT);
    $stmt->bindParam(':disc0lost', $_POST["disc0lost"], PDO::PARAM_INT);

			
	$stmt->bindParam(':glove0yaw', $_POST["glove0yaw"], PDO::PARAM_STR);
	$stmt->bindParam(':glove0pitch', $_POST["glove0pitch"], PDO::PARAM_STR);
    $stmt->bindParam(':glove0roll', $_POST["glove0roll"], PDO::PARAM_STR);
	$stmt->bindParam(':glove1yaw', $_POST["glove1yaw"], PDO::PARAM_STR);
	$stmt->bindParam(':glove1pitch', $_POST["glove1pitch"], PDO::PARAM_STR);
    $stmt->bindParam(':glove1roll', $_POST["glove1roll"], PDO::PARAM_STR);
	
	$stmt->bindParam(':latitude', $_POST["latitude"], PDO::PARAM_STR);
	$stmt->bindParam(':longitude', $_POST["longitude"], PDO::PARAM_STR);
    $stmt->bindParam(':accuracy', $_POST["accuracy"], PDO::PARAM_INT);
	
	$stmt->bindParam(':ip', $_SERVER['REMOTE_ADDR'], PDO::PARAM_STR);

    $stmt->execute();
			
			
	$color1 = '';
	$color2 = '';
	$msg = '';
	
    $stmt = $conn->prepare("SELECT * FROM req_color WHERE sent IS FALSE ORDER BY id ASC LIMIT 1");
    $stmt->execute();
    $req_color = $stmt->fetchAll();
    
	if (count($req_color) > 0) {
		$color1 = $req_color[0]["color1"];
		$color2 = $req_color[0]["color2"];
        
        $stmt = $conn->prepare("UPDATE req_color SET sent=TRUE WHERE id=(:id)");
        $stmt->bindParam(':id', $req_color[0]["id"], PDO::PARAM_INT);
        $stmt->execute();
    }
	
	$stmt = $conn->prepare("SELECT * FROM req_msg WHERE sent IS FALSE ORDER BY id ASC LIMIT 1");
    $stmt->execute();
    $req_msg = $stmt->fetchAll();
	
	if (count($req_msg) > 0) {
		$msg = $req_msg[0]["user_msg"];
        
        $stmt = $conn->prepare("UPDATE req_msg SET sent=TRUE WHERE id=(:id)");
        $stmt->bindParam(':id', $req_msg[0]["id"], PDO::PARAM_INT);
        $stmt->execute();
    }
	
    echo $msg . "\t" .	$color1 . "\t" . $color2 . "\n";
}
catch (PDOException $e) {
    print "Error: " . $e->getMessage();
}
$conn = null;

?>
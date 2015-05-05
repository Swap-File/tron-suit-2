<?php
include("connect.php");

try {
    $conn = new PDO("mysql:host=$servername;dbname=$dbname", $username, $password);
    $conn->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
    
    $stmt = $conn->prepare("SELECT * FROM req_color ORDER BY id DESC LIMIT 1");
    $stmt->execute();
    $req_color = $stmt->fetchAll();
	
	$stmt = $conn->prepare("SELECT * FROM req_msg ORDER BY id DESC LIMIT 1");
	$stmt->execute();
	$req_msg = $stmt->fetchAll();
	
    if (count($req_msg) > 0 && count($req_color) > 0){
		$newest_time = strtotime($req_color[0]["CreatedTime"]);
		$newest_ip =  $req_color[0]["ip"];
		$req_msg_time = strtotime($req_msg[0]["CreatedTime"]);
		
		if ($newest_time < $req_msg_time) {
			$newest_time = $req_msg_time;
			$newest_ip =  $req_msg[0]["ip"];
		}
		
		echo date('m-d-Y \a\t h:i:sa', $newest_time) . "\t" .
			$req_msg[0]["user_msg"] . "\t" .
			$req_color[0]["color1"] . "\t" .
			$req_color[0]["color2"]  . "\t" .
			$newest_ip . "\t";
		}
}
catch (PDOException $e) {
    print "Error: " . $e->getMessage();
}
$conn = null;

?>
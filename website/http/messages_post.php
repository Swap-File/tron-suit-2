<?php
include("connect.php");

try {
    $conn = new PDO("mysql:host=$servername;dbname=$dbname", $username, $password);
    // set the PDO error mode to exception
    $conn->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
    
    //check if person is in ban list
    $stmt = $conn->prepare("SELECT * FROM banlist WHERE ip=(:ip)");
    $stmt->bindParam(':ip', $_SERVER['REMOTE_ADDR'], PDO::PARAM_STR);
    $stmt->execute();
    $result = $stmt->fetchAll();
    
    //default score for newbies
    $score     = 0;
    $threshold = 5;
    $total     = 1;
    
    if (count($result) == 0) {
        //first request, create entry
        $stmt = $conn->prepare("INSERT INTO banlist (ip, score,total) VALUES (:ip, :score, :total)");
        $stmt->bindParam(':ip', $_SERVER['REMOTE_ADDR'], PDO::PARAM_STR);
        $stmt->bindParam(':score', $score, PDO::PARAM_INT);
        $stmt->bindParam(':total', $total, PDO::PARAM_INT);
        $stmt->execute();
    } else {
        //repeat customers start here
        
        // time in seconds between requests
        $changetime = time() - strtotime($result[0]["ModifiedTime"]);
        
        //get score
        $score = $result[0]['score'];
        $total = $result[0]['total'] + 1;
        //disallow time travel
        if ($changetime < 0) {
            $changetime = 0;
            $score      = 0; //if time travel happens, reset score
        }
        
        //drop score by 1 for every 10 seconds since last request
        $score = max($score - ($changetime / 10), 0);
        
        //increment score
        $score = $score + 1;
        
        //punish high scores
        if ($score > (2 * $threshold)) {
            $score = min($score * $score,14400);
        }
        
        //store new score
        $stmt = $conn->prepare("UPDATE banlist SET score=(:score), total=(:total) WHERE ip=(:ip)");
        $stmt->bindParam(':ip', $_SERVER['REMOTE_ADDR'], PDO::PARAM_STR);
        $stmt->bindParam(':total', $total, PDO::PARAM_INT);
        $stmt->bindParam(':score', $score, PDO::PARAM_INT);
        $stmt->execute();
    }
    
    if ($score <= $threshold) {
        
        if ($_POST["color2"] != "#000000" && $_POST["color2"] != "#000000") {
		
            // prepare sql and bind parameters
            $stmt = $conn->prepare("INSERT INTO req_color (ip, color1, color2) 
            VALUES (:ip, :color1, :color2)");
            
            $stmt->bindParam(':color2', strtoupper($_POST["color2"]), PDO::PARAM_STR);
            $stmt->bindParam(':color1', strtoupper($_POST["color1"]), PDO::PARAM_STR);
            $stmt->bindParam(':ip', $_SERVER['REMOTE_ADDR'], PDO::PARAM_STR);
            $stmt->execute();
        }
        
        if (strlen(trim($_POST["new_text"])) > 0) {
            // prepare sql and bind parameters
            $stmt = $conn->prepare("INSERT INTO req_msg (user_msg, ip) 
        VALUES (:user_msg, :ip)");
            
            $stmt->bindParam(':user_msg', $_POST["new_text"], PDO::PARAM_STR);
            $stmt->bindParam(':ip', $_SERVER['REMOTE_ADDR'], PDO::PARAM_STR);
            $stmt->execute();
        }
    }
    print max($score - $threshold, 0);
}
catch (PDOException $e) {
    print "Error: " . $e->getMessage();
}
$conn = null;
?>

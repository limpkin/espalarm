<html><head><title>Test</title>
<link rel="stylesheet" type="text/css" href="style.css">
</head>
<body>
<div id="main">
<h1>LEDs control</h1>
<p>
Current Red PWM: %redpwm% percent<br>
Current Blue PWM: %bluepwm% percent<br>
Current Green PWM: %greenpwm% percent<br>
</p>
<form method="post" action="led.cgi">
New Red PWM val:&nbsp;&nbsp;&nbsp;&nbsp; <input type="text" name="redval" value="%redpwm%"><br>
New Blue PWM val:&nbsp;&nbsp;&nbsp; <input type="text" name="blueval" value="%bluepwm%"><br>
New Green PWM val: <input type="text" name="greenval" value="%greenpwm%"><br><br>
<input type="submit" name="submit" value="Submit">
</form>
</div>
</body></html>

<html><head><title>Test</title>
<link rel="stylesheet" type="text/css" href="style.css">
</head>
<body>
<div id="main">
Current date: <b>%curtime%</b><br>
Time before alarm: <b>%timebeforealarm%</b><br>
<form method="post" action="discardalarm.cgi">
<input type="submit" name="submit" value="Discard Next Alarm"><br><br>
</form>
<h1>Alarm Settings</h1>
<form method="post" action="alarm.cgi">
<input type="checkbox" name="monday" value="monday" %monday%> Monday<br>
<input type="checkbox" name="tuesday" value="tuesday" %tuesday%> Tuesday<br>
<input type="checkbox" name="wednesday" value="wednesday" %wednesday%> Wednesday<br>
<input type="checkbox" name="thursday" value="thursday" %thursday%> Thursday<br>
<input type="checkbox" name="friday" value="friday" %friday%> Friday<br>
<input type="checkbox" name="saturday" value="saturday" %saturday%> Saturday<br>
<input type="checkbox" name="sunday" value="sunday" %sunday%> Sunday<br><br>
Hours:&nbsp;&nbsp;&nbsp;&nbsp; <input type="text" name="hours" value="%hours%"><br>
Minutes:&nbsp; <input type="text" name="minutes" value="%minutes%"><br><br>
Alarm color, red percentage:&nbsp;&nbsp;&nbsp; <input type="text" name="redpct" value="%redpct%"><br>
Alarm color, blue percentage:&nbsp;&nbsp; <input type="text" name="bluepct" value="%bluepct%"><br>
Alarm color, green percentage: <input type="text" name="greenpct" value="%greenpct%"><br><br>
2 hours before alarm, blue PWM value:&nbsp;&nbsp;&nbsp;<input type="text" name="blueprep" value="%blueprep%"><br>
1 hours before alarm, green PWM value:&nbsp;<input type="text" name="greenprep" value="%greenprep%"><br>
30 mins before alarm, red PWM value:&nbsp;&nbsp;&nbsp;&nbsp;<input type="text" name="redprep" value="%redprep%"><br><br>
<input type="submit" name="submit" value="Submit New Settings">
</form>
</div>
</body></html>

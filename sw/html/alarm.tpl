<html><head><title>Test</title>
<link rel="stylesheet" type="text/css" href="style.css">
</head>
<body>
<div id="main">
<h1>Alarm Setting</h1>
Current date: <b>%curtime%</b><br><br>
<form method="post" action="alarm.cgi">
<input type="checkbox" name="monday" value="monday" %monday%> Monday<br>
<input type="checkbox" name="tuesday" value="tuesday" %tuesday%> Tuesday<br>
<input type="checkbox" name="wednesday" value="wednesday" %wednesday%> Wednesday<br>
<input type="checkbox" name="thursday" value="thursday" %thursday%> Thursday<br>
<input type="checkbox" name="friday" value="friday" %friday%> Friday<br>
<input type="checkbox" name="saturday" value="saturday" %saturday%> Saturday<br>
<input type="checkbox" name="sunday" value="sunday" %sunday%> Sunday<br><br>
Hours:&nbsp;&nbsp;&nbsp;&nbsp; <input type="text" name="hours" value="%hours%"><br>
Minutes:&nbsp; <input type="text" name="minutes" value="%minutes%"><br>
<input type="submit" name="submit" value="Submit">
</form>
</div>
</body></html>

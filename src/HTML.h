// The following raw literal is holding the web page that is used when the switch is configured. This web page displays
// a visual representation of the switch that lets you turn the light on and off. It also updates the on screen switch
// if you push the button
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>BeBe Smart Switch</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 1.5rem;}
    p {font-size: 3.0rem;}
    div#bottom-right {position: absolute; bottom: -55px; right: 0px}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px; transform: rotate(-90deg)} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 34px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 68px}
    input:checked+.slider {background-color: #2196F3}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
    .wave {
        display: inline-block;
        border: 10px solid transparent;
        border-top-color: currentColor;
        border-radius: 50%%;
        border-style: solid;
        margin: 5px;
    }
    .waveStrength-3 .wv4.wave,
    .waveStrength-2 .wv4.wave,
    .waveStrength-2 .wv3.wave,
    .waveStrength-1 .wv4.wave,
    .waveStrength-1 .wv3.wave,
    .waveStrength-1 .wv2.wave {
        border-top-color: #eee;
    }
  </style>
</head>
<body>
  <h2>BeBe smart light switch</h2>
  <div id=bottom-right>%SIGNALSTRENGTH%</div>
  %BUTTONPLACEHOLDER%
  <br><br><br>
  <a href="/settings" >Settings Page</a>
<script>function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/refresh?state=1", true); }
  else { xhr.open("GET", "/refresh?state=0", true); }
  xhr.send();
}

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var inputChecked;
      var outputStateM;
      if( this.responseText == 1){ 
        inputChecked = true;
        outputStateM = "On";
      }
      else { 
        inputChecked = false;
        outputStateM = "Off";
      }
      document.getElementById("output").checked = inputChecked;
      document.getElementById("outputState").innerHTML = outputStateM;
    }
  };
  xhttp.open("GET", "/state", true);
  xhttp.send();
}, 1000 ) ;
</script>
</body>
</html>
)rawliteral";

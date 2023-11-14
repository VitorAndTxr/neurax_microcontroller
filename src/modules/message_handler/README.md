# Data model

Data will be sent and received to app using JSON. Each message contains a code, which defines the JSON's structure, and a method. Methods can be:

| Method | Meaning |
| --- | ---- |
| r | Read | 
| w | Write | 
| x | Execute |

A packet example is presented bellow.
 ```JSON
 {
  "cd": 7,
  "mt": "w",
  "bd": {
    "a": 0.0, //amplitude
    "f": 0.0, // frequency
    "pw": 0.0, //pulse width
    "df": 00, //difficulty
    "pd": 00, //pulse duration
  },
 }
 ```

Depending on the message's code and method, the message may contain a body with additional information needed. The table bellow explains the codification of messages.


<table>
<thead>
  <tr>
    <th>Code</th>
    <th>Description</th>
    <th>Method</th>
    <th>Origin</th>
    <th>Destination</th>
    <th>Body content</th>
    <th>Notes</th>
  </tr>
</thead>
<tbody>
  <tr>
    <td rowspan="3">1</td>
    <td rowspan="3">Gyroscope reading</td>
    <td>r</td>
    <td>App</td>
    <td>ESP32</td>
    <td>No body.</td>
    <td></td>
  </tr>
  <tr>
    <td>x</td>
    <td>App</td>
    <td>ESP32</td>
    <td>No body.</td>
    <td></td>
  </tr>
  <tr>
    <td>w</td>
    <td>ESP32</td>
    <td>App</td>
    <td>a: angle, degrees (float)</td>
    <td></td>
  </tr>
  <tr>
    <td>2</td>
    <td>Start session</td>
    <td>x</td>
    <td>App</td>
    <td>ESP32</td>
    <td>No body.</td>
    <td></td>
  </tr>
  <tr>
    <td>3</td>
    <td>Stop session</td>
    <td>x</td>
    <td>App</td>
    <td>ESP32</td>
    <td>No body.</td>
    <td></td>
  </tr>
  <tr>
    <td>4</td>
    <td>Pause session</td>
    <td>x</td>
    <td>App</td>
    <td>ESP32</td>
    <td>No body.</td>
    <td></td>
  </tr>
  <tr>
    <td>5</td>
    <td>Resume session</td>
    <td>x</td>
    <td>App</td>
    <td>ESP32</td>
    <td>No body.</td>
    <td></td>
  </tr>
  <tr>
    <td>6</td>
    <td>Single stimuli generation</td>
    <td>x</td>
    <td>App</td>
    <td>ESP32</td>
    <td>No body.</td>
    <td>If a package code 7 has been received before, those parameters are used. Otherwise, fw uses default values.</td>
  </tr>
  <tr>
    <td>7</td>
    <td>FES parameters and difficulty</td>
    <td>w</td>
    <td>App</td>
    <td>ESP32</td>
    <td>a: amplitude, V (float)<br>f: frequency, Hz (float)<br>pw: pulse width, ms (float)<br>df: difficulty, % (int)<br>pd: stimuli duration, s (int)</td>
    <td></td>
  </tr>
  <tr>
    <td>8</td>
    <td>Stimuli parameters and session status</td>
    <td>w</td>
    <td>ESP32</td>
    <td>App</td>
    <td>parameters: { (same as FES body) }<br>status:<br>{<br>csa: complete stimuli amount<br>isa: interrupted stimuli amount<br>tlt: time of last trigger (in ms after session start)<br>sd: session duration (in ms since session start)<br>}</td>
    <td></td>
  </tr>
  <tr>
    <td rowspan="3">9</td>
    <td rowspan="3">Trigger</td>
    <td>w</td>
    <td>ESP32</td>
    <td>App</td>
    <td>No body.</td>
    <td>sEMG module detected a trigger.</td>
    <tr>
    <td>x</td>
    <td>App</td>
    <td>ESP32</td>
    <td>No body.</td>
    <td>App request a trigger detection test to ESP32.</td>
    </tr>
  </tr>
</tbody>
</table>

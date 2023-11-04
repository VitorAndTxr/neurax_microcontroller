# Data model

Data will be sent and received to app using JSON. The table bellow explains the codification of messages.


<table>
<thead>
  <tr>
    <th>Origin</th>
    <th>Method</th>
    <th>Code</th>
    <th>Description</th>
  </tr>
</thead>
<tbody>
  <tr>
    <td rowspan="3">ESP32</td>
    <td rowspan="2">w</td>
    <td>2</td>
    <td>Gyroscope reading</td>
  </tr>
  <tr>
    <td>3</td>
    <td>Stimuli params and session status</td>
  </tr>
  <tr>
    <td>a</td>
    <td>y</td>
    <td>ACK</td>
  </tr>
  <tr>
    <td rowspan="6">App</td>
    <td rowspan="2">w</td>
    <td>1</td>
    <td>Send FES parameters (and difficulty) to ESP</td>
  </tr>
  <tr>
    <td>4</td>
    <td>Handshake</td>
  </tr>
  <tr>
    <td rowspan="3">x</td>
    <td>1</td>
    <td>Start session</td>
  </tr>
  <tr>
    <td>2</td>
    <td>Stop stimulation</td>
  </tr>
  <tr>
    <td>3</td>
    <td>Single stimuli generation (test)</td>
  </tr>
  <tr>
    <td>a</td>
    <td>y</td>
    <td>ACK</td>
  </tr>
</tbody>
</table>
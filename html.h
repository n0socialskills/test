/**
 * @file html.h
 * @brief Contains the HTML, CSS, and JavaScript for the web interface.
 *
 *** Change <h2>Pirate Controls THCS S3 Sensor Controller</h2> To Match Sensor # ***
 */

#ifndef HTML_H
#define HTML_H

#include <Arduino.h> // Required for PROGMEM attribute

// The entire HTML, CSS, and JavaScript for the web interface is stored in flash memory (PROGMEM).
// R"rawliteral(...)rawliteral" is used for a raw string literal, avoiding the need to escape special characters.
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>Pirate Controls THC-S Soil Sensor</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <script src="https://cdn.jsdelivr.net/npm/chartjs-adapter-date-fns"></script>
  <style>
    /* Basic styling for the page */
    :root {
      --bg-color: #121212; /* Dark background */
      --text-color: #e0e0e0; /* Light text */
      --card-bg: #1e1e1e; /* Background for container */
      --primary: #3498db; /* Primary color (blue) */
      --primary-hover: #2980b9; /* Darker blue on hover */
      --blue-btn: #3498db;
      --blue-btn-hover: #2980b9;
      --connected: #4CAF50; /* Green for connected status */
      --disconnected: #F44336; /* Red for disconnected status */
      --section-bg: #252525; /* Background for data rows and input groups */
      --warning: #ff9800; /* Orange for warnings */
      --danger: #F44336; /* Red for danger/reset buttons */
      --tab-bg: #1a1a1a; /* Background for inactive tabs */
      --tab-active-bg: #252525; /* Background for the active tab */
      --border-color: #444; /* Border color */
    }
    body {
      font-family: Arial, sans-serif;
      margin: 0;
      padding: 20px;
      background-color: var(--bg-color);
      color: var(--text-color);
      line-height: 1.6;
    }
    .container {
      max-width: 800px;
      margin: 0 auto;
      background: var(--card-bg);
      padding: 0; /* No padding on container itself, padding on sections */
      border-radius: 8px;
      box-shadow: 0 2px 10px rgba(0,0,0,0.3);
      overflow: hidden; /* Ensure child elements stay within rounded corners */
    }
    .logo-container {
     text-align: center; 
     padding: 15px 0 15px 0; /* Adjust padding as needed */
     background: var(--tab-bg); 
     border-bottom: 1px solid #333; 
}
    .logo-container h2 { /* Style the new header */
    margin: 0; /* Remove default margins */
    font-size: 1.3em; /* Adjust size as needed */
    color: white; /* Optional: Use primary color */
}
    h1 { /* Used for the main title, but hidden currently */
      color: white;
      text-align: center;
      margin: 0;
      padding: 20px;
      font-size: 1.5em; /* Adjusted size */
      border-bottom: 1px solid #333;
    }
    .section {
      padding: 15px 20px;
      display: none; /* Sections are hidden by default */
    }
    .section.active {
      display: block; /* Active section is shown */
    }
    .section-title {
      color: var(--primary);
      margin-top: 20px;
      margin-bottom: 15px;
      font-size: 1.1em; /* Adjusted size */
      border-bottom: 1px solid var(--border-color);
      padding-bottom: 5px;
    }
    .data-row {
      display: flex;
      justify-content: space-between;
      align-items: center; /* Vertically align items */
      margin: 10px 0;
      padding: 10px;
      border-radius: 4px;
      background: var(--section-bg);
      font-size: 0.9em; /* Slightly smaller font for data rows */
    }
    /* --- Style for grouping input sections --- */
    .input-group {
        background-color: var(--section-bg); /* Match data-row background */
        border: 1px solid var(--border-color); /* Add a subtle border */
        border-radius: 6px; /* Slightly rounded corners */
        padding: 15px; /* Add padding inside the group */
        margin-top: 10px; /* Space below the preceding title */
        margin-bottom: 20px; /* Space after the group */
    }
    /* Specific styling for input rows */
    .calibration-row, .mqtt-row, .wifi-row {
      display: flex;
      align-items: center;
      margin: 15px 0; /* Keep vertical spacing between rows */
      gap: 10px; /* Add gap between label and input */
    }
    .calibration-label, .mqtt-label, .wifi-label {
      width: 150px; /* Fixed width for labels */
      font-weight: bold;
      flex-shrink: 0; /* Prevent label from shrinking */
      font-size: 0.9em;
    }
    .mqtt-input, .calibration-input, .wifi-input {
      flex-grow: 1; /* Allow input to take remaining space */
      background: var(--bg-color);
      border: 1px solid var(--border-color);
      color: var(--text-color);
      padding: 8px;
      border-radius: 4px;
      font-size: 0.9em;
    }
    .data-label {
      font-weight: bold;
    }
    .data-value {
      text-align: right;
    }
    .unit {
      color: #aaa;
      margin-left: 5px;
    }
    .status-badge {
      padding: 3px 8px; /* Adjusted padding */
      border-radius: 4px;
      font-size: 0.85em; /* Adjusted size */
      display: inline-block;
      color: white;
      font-weight: bold;
      min-width: 80px; /* Ensure minimum width */
      text-align: center;
    }
    .connected {
      background: var(--connected);
    }
    .disconnected {
      background: var(--disconnected);
    }
    /* Styling for the bottom control buttons */
    .controls {
      padding: 15px 20px;
      display: flex;
      gap: 10px;
      justify-content: center;
      flex-wrap: wrap;
      border-top: 1px solid #333;
      background: var(--tab-bg); /* Match tab background */
    }
    button {
      background: var(--blue-btn);
      color: white;
      border: none;
      padding: 8px 16px;
      border-radius: 4px;
      cursor: pointer;
      font-size: 0.9em; /* Adjusted size */
      transition: background 0.3s;
    }
    button:hover {
      background: var(--blue-btn-hover);
    }
    .danger-btn {
      background: var(--danger);
    }
    .danger-btn:hover {
      background: #d32f2f; /* Darker red on hover */
    }
    /* Styling for save buttons within settings sections */
    .save-calibration, .save-mqtt, .save-wifi {
      width: 100%;
      margin-top: 15px; /* Increased margin */
      padding: 10px; /* Larger padding */
    }
    /* Styling for modal popups */
    .modal {
      display: none; /* Hidden by default */
      position: fixed;
      z-index: 100;
      left: 0;
      top: 0;
      width: 100%;
      height: 100%;
      background-color: rgba(0,0,0,0.7); /* Semi-transparent background */
      align-items: center; /* Center vertically */
      justify-content: center; /* Center horizontally */
    }
    .modal-content {
      background-color: var(--card-bg);
      padding: 25px;
      border-radius: 8px;
      width: 90%;
      max-width: 450px;
      text-align: center;
      box-shadow: 0 5px 15px rgba(0,0,0,0.5);
    }
    .modal-buttons {
      display: flex;
      justify-content: center;
      gap: 15px;
      margin-top: 25px;
    }
    /* Styling for tabs */
    .tabs {
      display: flex;
      border-bottom: 1px solid #333;
      background: var(--tab-bg);
      position: sticky;
      top: 0;
      z-index: 10;
    }
    .tab {
      padding: 12px 10px;
      cursor: pointer;
      transition: background 0.3s, color 0.3s, border-bottom 0.3s;
      border-bottom: 3px solid transparent;
      text-align: center;
      flex: 1;
      font-size: 0.85em;
    }
    .tab:hover {
      background: rgba(255,255,255,0.05);
    }
    .tab.active {
      background: var(--tab-active-bg);
      border-bottom: 3px solid var(--primary);
      color: var(--primary);
      font-weight: bold;
    }
    /* Styling for the status/time bar */
    .status-time {
      text-align: center;
      padding: 8px;
      font-size: 0.85em;
      color: #888;
      border-bottom: 1px solid #333;
      background: var(--tab-bg);
    }
    /* Styling for chart controls */
     #charts .chart-controls { /* Added space */
      display: flex;
      gap: 10px;
      justify-content: center;
      margin-bottom: 15px;
    }
     #sensorChart { /* Added space */
      background: var(--section-bg);
      border-radius: 4px;
      padding: 10px;
      max-height: 400px;
    }
    /* Status message styling */
    .status-message {
        margin-top: 15px;
        text-align: center;
        font-weight: bold;
        min-height: 1.2em;
    }
    .success-message { color: var(--connected); }
    .error-message { color: var(--danger); }
    .info-message { color: var(--primary); }

    @media (max-width: 500px) {
    /* Keep the rules for rows and labels as they were: */
    .calibration-row, .wifi-row, .mqtt-row {
    flex-direction: column; 
    align-items: stretch; /* Or align-items: center; if you preferred that */
    gap: 5px; 
    }
    .calibration-label, .wifi-label, .mqtt-label {
    width: auto; 
    text-align: left; 
    margin-bottom: 3px; 
    }

    /* Style for TEXT and PASSWORD inputs */
    .wifi-input, /* All WiFi inputs are text/password */
    .mqtt-input[type="text"], 
    .mqtt-input[type="password"] {
     width: auto;      
     max-width: 150px; /* Make TEXT boxes shorter (adjust value as needed) */
    }

    /* Style for NUMBER inputs */
    .calibration-input, /* All Calibration inputs are numbers */
    .mqtt-input[type="number"] { /* Specifically the MQTT port */
     width: auto;
     max-width: 150px; /* Make TEXT boxes shorter (adjust value as needed) */
    }

    /* Adjust save buttons if needed */
    .save-calibration, .save-mqtt, .save-wifi {
     margin-top: 20px; 
    }
}

  </style>
</head>
<body>
  <div class="container">
    <div class="logo-container">
     <h2>Pirate Controls THCS S3 Sensor Controller</h2>
    </div>
    <div class="status-time">
      <span id="current-time">--</span>
    </div>

    <div class="tabs">
      <div class="tab active" data-tab="sensor">THC-S Sensor</div>
      <div class="tab" data-tab="charts">Graph</div>
      <div class="tab" data-tab="wifi-settings">WiFi</div>
      <div class="tab" data-tab="mqtt-settings">MQTT</div>
      <div class="tab" data-tab="calibration">Calibration</div>
    </div>

    <div id="sensor" class="section active">
      <h3 class="section-title">Live Sensor Readings</h3>
      <div class="data-row">
        <span class="data-label">Sensor Status:</span>
        <span class="data-value"><span id="sensor-status" class="status-badge disconnected">--</span></span>
      </div>
      <div class="data-row">
        <span class="data-label">VWC:</span>
        <span class="data-value"><span id="humidity">--</span><span class="unit">%</span></span>
      </div>
      <div class="data-row">
        <span class="data-label">Temperature:</span>
        <span class="data-value">
          <span id="temperature">--</span>
          <span class="unit" id="temp-unit">&deg;C</span>
        </span>
      </div>
       <div class="data-row">
        <span class="data-label">Bulk EC:</span>
        <span class="data-value"><span id="ec">--</span><span class="unit">&micro;S/cm</span></span>
      </div>
       <div class="data-row">
        <span class="data-label">Raw EC:</span>
        <span class="data-value"><span id="raw_ec">--</span><span class="unit">&micro;S/cm</span></span>
      </div>
      <div class="data-row">
        <span class="data-label">Pore Water EC:</span>
        <span class="data-value"><span id="pw_ec">--</span><span class="unit">dS/m</span></span>
      </div>

      <h3 class="section-title">System Status</h3>
       <div class="data-row">
        <span class="data-label">WiFi Status:</span>
        <span class="data-value"><span id="wifi-status-main" class="status-badge disconnected">--</span></span>
      </div>
       <div class="data-row">
        <span class="data-label">IP Address:</span>
        <span class="data-value"><span id="current-ip-main">--</span></span>
      </div>
      <div class="data-row">
        <span class="data-label">MQTT Status:</span>
        <span class="data-value"><span id="mqtt-status-main" class="status-badge disconnected">--</span></span>
      </div>
      <div class="data-row">
        <span class="data-label">MQTT Topic:</span>
        <span class="data-value"><span id="mqtt-topic-main">--</span></span>
      </div>
      <div class="data-row">
        <span class="data-label">Uptime:</span>
        <span class="data-value"><span id="esp-uptime">--</span></span>
      </div>
    </div>

    <div id="charts" class="section">
      <h3 class="section-title">Historical Sensor Data</h3>
      <div class="chart-controls">
        <button onclick="updateChart(36)">36h</button>
        <button onclick="updateChart(24)">24h</button>
        <button onclick="updateChart(12)">12h</button>
      </div>
      <canvas id="sensorChart"></canvas>
    </div>

    <div id="wifi-settings" class="section">
       <h3 class="section-title">Current WiFi Status</h3>
       <div class="data-row">
        <span class="data-label">WiFi Status:</span>
        <span class="data-value"><span id="wifi-status" class="status-badge disconnected">--</span></span>
      </div>
      <div class="data-row">
        <span class="data-label">Current SSID:</span>
        <span class="data-value"><span id="current-ssid">--</span></span>
      </div>
      <div class="data-row">
        <span class="data-label">IP Address:</span>
        <span class="data-value"><span id="current-ip">--</span></span>
      </div>
       <div class="data-row">
        <span class="data-label">Signal Strength (RSSI):</span>
        <span class="data-value"><span id="current-rssi">--</span> dBm</span>
      </div>

      <h3 class="section-title">Update WiFi Credentials</h3>
      <div class="input-group">
          <div class="wifi-row">
            <span class="wifi-label">New SSID:</span>
            <input type="text" id="wifi-ssid" class="wifi-input" placeholder="eg. Fuck Aroya">
          </div>
          <div class="wifi-row">
            <span class="wifi-label">New Password:</span>
            <input type="password" id="wifi-password" class="wifi-input" placeholder="eg. Fuck TrolMaster">
          </div>
          <button id="save-wifi" class="save-wifi" onclick="saveWiFiSettings()">Update WiFi & Reboot</button>
      </div>
      <div id="wifi-status-msg" class="status-message"></div>
    </div>

    <div id="mqtt-settings" class="section">
      <h3 class="section-title">Current MQTT Status</h3>
      <div class="data-row">
        <span class="data-label">MQTT Status:</span>
        <span class="data-value"><span id="mqtt-status" class="status-badge disconnected">--</span></span>
      </div>
      <div class="data-row">
        <span class="data-label">Server:</span>
        <span class="data-value"><span id="mqtt-server-display">--</span></span>
      </div>
      <div class="data-row">
        <span class="data-label">Port:</span>
        <span class="data-value"><span id="mqtt-port-display">--</span></span>
      </div>
      <div class="data-row">
        <span class="data-label">Username:</span>
        <span class="data-value"><span id="mqtt-username-display">--</span></span>
      </div>
      <div class="data-row">
        <span class="data-label">Topic:</span>
        <span class="data-value"><span id="mqtt-topic-display">--</span></span>
      </div>

      <h3 class="section-title">Update MQTT Settings (Connection)</h3>
      <div class="input-group">
          <div class="mqtt-row">
            <span class="mqtt-label">Server:</span>
            <input type="text" id="mqtt-server" class="mqtt-input" placeholder="e.g., 192.168.1.420">
          </div>
          <div class="mqtt-row">
            <span class="mqtt-label">Port:</span>
            <input type="number" id="mqtt-port" class="mqtt-input" placeholder="e.g., 1883">
          </div>
          <div class="mqtt-row">
            <span class="mqtt-label">Username:</span>
            <input type="text" id="mqtt-username" class="mqtt-input" placeholder="eg. P@$$W0RD">
          </div>
          <div class="mqtt-row">
            <span class="mqtt-label">Password:</span>
            <input type="password" id="mqtt-password" class="mqtt-input" placeholder="eg. PiratesRule">
          </div>
          <button id="save-mqtt" class="save-mqtt" onclick="saveMQTTSettings()">Save Connection Settings & Reconnect</button>
      </div>

      <h3 class="section-title" style="margin-top: 25px;">Update MQTT Topic </h3>
       <div class="input-group">
           <div class="mqtt-row">
            <span class="mqtt-label">Topic:</span>
            <input type="text" id="mqtt-topic" class="mqtt-input" placeholder="e.g., sensors/thcs/s1">
          </div>
            <button id="save-mqtt-topic" class="save-mqtt" onclick="saveMQTTTopic()" title="Save MQTT Topic">Save Topic</button>
       </div>
      <div id="mqtt-status-msg" class="status-message"></div>
    </div>

     <div id="calibration" class="section">
       <h3 class="section-title">Current Calibration Values</h3>
       <div class="data-row">
        <span class="data-label">EC Slope:</span>
        <span class="data-value"><span id="current-ec-slope">--</span></span>
      </div>
       <div class="data-row">
        <span class="data-label">EC Intercept:</span>
        <span class="data-value"><span id="current-ec-intercept">--</span></span>
      </div>
       <div class="data-row">
        <span class="data-label">Temp. Coefficient:</span>
        <span class="data-value"><span id="current-temp-coeff">--</span></span>
      </div>

      <h3 class="section-title">Update EC Calibration</h3>
      <div class="input-group">
          <div class="calibration-row">
            <span class="calibration-label">EC Slope:</span>
            <input type="number" step="0.01" id="ec-slope" class="calibration-input" placeholder="e.g., 1.0">
          </div>
          <div class="calibration-row">
            <span class="calibration-label">EC Intercept:</span>
            <input type="number" step="1" id="ec-intercept" class="calibration-input" placeholder="e.g., 0">
          </div>
          <div class="calibration-row">
            <span class="calibration-label">Temp. Coefficient:</span>
            <input type="number" step="0.001" id="temp-coeff" class="calibration-input" placeholder="e.g., 0.02">
          </div>
          <button id="save-calibration" class="save-calibration" onclick="saveCalibrationSettings()">Save Calibration Settings</button>
       </div> <div id="calibration-status-msg" class="status-message"></div>
    </div>
     <div class="controls">
      <button onclick="toggleTempUnit()" class="temp-toggle" title="Toggle Temperature Unit">&deg;C/&deg;F</button>
      <button onclick="openWebSerial()" class="webserial-btn" title="Open WebSerial Console">WebSerial</button>
      <button onclick="rebootDevice()" title="Reboot the ESP32">Reboot</button>
      <button onclick="factoryReset()" class="danger-btn" title="Reset all settings to defaults">Factory Reset</button>
    </div>
  </div>

  <div id="mqtt-modal" class="modal">
    <div class="modal-content">
      <h3>Confirm MQTT Settings</h3>
      <p>Save these MQTT settings and attempt to reconnect?</p>
      <div id="mqtt-values" style="text-align:left; margin:15px 0; padding:10px; background:var(--section-bg); border-radius:4px;"></div>
      <div class="modal-buttons">
        <button onclick="confirmMQTTSettings(true)">Save & Reconnect</button>
        <button onclick="confirmMQTTSettings(false)">Cancel</button>
      </div>
    </div>
  </div>

  <div id="calibration-modal" class="modal">
    <div class="modal-content">
      <h3>Confirm Calibration Settings</h3>
      <p>Are you sure you want to save these calibration values?</p>
      <div id="calibration-values" style="text-align:left; margin:15px 0; padding:10px; background:var(--section-bg); border-radius:4px;"></div>
      <div class="modal-buttons">
        <button onclick="confirmCalibrationSettings(true)">Save</button>
        <button onclick="confirmCalibrationSettings(false)">Cancel</button>
      </div>
    </div>
  </div>

  <div id="wifi-modal" class="modal">
    <div class="modal-content">
      <h3>Confirm WiFi Settings</h3>
      <p>Update WiFi settings and reboot the device?</p>
      <p>(Device will disconnect and attempt to connect to the new network)</p>
      <div id="wifi-values" style="text-align:left; margin:15px 0; padding:10px; background:var(--section-bg); border-radius:4px;"></div>
      <div class="modal-buttons">
        <button onclick="confirmWiFiSettings(true)">Update & Reboot</button>
        <button onclick="confirmWiFiSettings(false)">Cancel</button>
      </div>
    </div>
  </div>

  <div id="reboot-modal" class="modal">
    <div class="modal-content">
      <h3>Confirm Reboot</h3>
      <p>Are you sure you want to reboot the device?</p>
      <div class="modal-buttons">
        <button onclick="confirmReboot(true)">Reboot</button>
        <button onclick="confirmReboot(false)">Cancel</button>
      </div>
    </div>
  </div>

  <div id="reset-modal" class="modal">
    <div class="modal-content">
      <h3>Confirm Factory Reset</h3>
      <p style="color: var(--warning); font-weight:bold;">WARNING: This will erase all saved settings (WiFi, MQTT, Calibration) and historical data!</p>
      <p>Are you sure you want to proceed?</p>
      <div class="modal-buttons">
        <button onclick="confirmFactoryReset(true)" class="danger-btn">Reset Now</button>
        <button onclick="confirmFactoryReset(false)">Cancel</button>
      </div>
    </div>
  </div>

  <script>
      // --- JavaScript includes the ws.send({type:'get_config'}) on connect ---
      // --- loadConfigSettings function MUST update current-ec-* elements AGAIN ---
      let ws; let useCelsius = true; let currentTempC = 0; let chart; let configLoaded = false;
      function initChart() { const ctx = document.getElementById('sensorChart').getContext('2d'); chart = new Chart(ctx, { type: 'line', data: { datasets: [{ label: 'VWC (%)', yAxisID: 'vwc', borderColor: 'rgb(75, 192, 192)', tension: 0.1, pointRadius: 1, borderWidth: 2 }, { label: 'pwEC (dS/m)', yAxisID: 'pwec', borderColor: 'rgb(255, 99, 132)', tension: 0.1, pointRadius: 1, borderWidth: 2 }, { label: 'Temperature', yAxisID: 'temp', borderColor: 'rgb(255, 159, 64)', tension: 0.1, pointRadius: 1, borderWidth: 2 }] }, options: { responsive: true, maintainAspectRatio: false, interaction: { mode: 'index', intersect: false, }, plugins: { legend: { position: 'top', labels: { color: '#e0e0e0' } }, tooltip: { mode: 'index', intersect: false, callbacks: { label: function(context) { let label = context.dataset.label || ''; if (label) { label += ': '; } if (context.parsed.y !== null) { if (context.datasetIndex === 0) { label += context.parsed.y.toFixed(1) + ' %'; } else if (context.datasetIndex === 1) { label += context.parsed.y.toFixed(2) + ' dS/m'; } else if (context.datasetIndex === 2) { label += context.parsed.y.toFixed(1) + (useCelsius ? ' °C' : ' °F'); } else { label += context.parsed.y; } } return label; } } } }, scales: { x: { type: 'time', time: { unit: 'hour', tooltipFormat: 'MMM d, HH:mm:ss', displayFormats: { hour: 'HH:mm', day: 'MMM d' } }, grid: { color: '#333' }, ticks: { color: '#e0e0e0', maxRotation: 0, autoSkip: true, autoSkipPadding: 15 } }, vwc: { type: 'linear', position: 'left', title: { display: true, text: 'VWC (%)', color: '#e0e0e0' }, grid: { color: '#444' }, ticks: { color: '#e0e0e0' }, }, pwec: { type: 'linear', position: 'right', title: { display: true, text: 'pwEC (dS/m)', color: '#e0e0e0' }, grid: { drawOnChartArea: false }, ticks: { color: '#e0e0e0' }, }, temp: { type: 'linear', position: 'right', title: { display: true, text: `Temperature (${useCelsius ? '°C' : '°F'})`, color: '#e0e0e0' }, grid: { drawOnChartArea: false }, ticks: { color: '#e0e0e0' }, } } } }); }
      function updateChart(hours) { if (ws && ws.readyState === WebSocket.OPEN) { console.log(`Requesting chart data for ${hours} hours`); ws.send(JSON.stringify({ type: 'get_chart_data', hours: hours })); } else { console.warn("WebSocket not open. Cannot request chart data."); } }
      function updateClock() { const now = new Date(); const year = now.getFullYear(); const month = (now.getMonth() + 1).toString().padStart(2, '0'); const day = now.getDate().toString().padStart(2, '0'); const hour = now.getHours().toString().padStart(2, '0'); const minute = now.getMinutes().toString().padStart(2, '0'); const second = now.getSeconds().toString().padStart(2, '0'); document.getElementById('current-time').textContent = `${year}-${month}-${day} ${hour}:${minute}:${second}`; }
      function setupTabs() { const tabs = document.querySelectorAll('.tab'); tabs.forEach(tab => { tab.addEventListener('click', () => { tabs.forEach(t => t.classList.remove('active')); document.querySelectorAll('.section').forEach(s => s.classList.remove('active')); tab.classList.add('active'); const tabId = tab.getAttribute('data-tab'); const section = document.getElementById(tabId); if (section) { section.classList.add('active'); } else { console.error(`Section with id "${tabId}" not found.`); } if (tabId === 'charts' && chart) { updateChart(24); setTimeout(() => { if (chart) chart.resize(); }, 50); } }); }); }

      const connectWebSocket = () => {
          const wsProtocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:'; const wsPath = window.location.pathname.startsWith('/webui') ? '/webui/ws' : '/ws'; const wsUrl = `${wsProtocol}//${window.location.host}${wsPath}`; console.log(`Attempting WebSocket connection to: ${wsUrl}`); ws = new WebSocket(wsUrl);
          ws.onopen = function() { console.log('WebSocket connected'); if (ws && ws.readyState === WebSocket.OPEN) { ws.send(JSON.stringify({ type: 'get_config' })); console.log('Requested config from ESP32.'); } };
          ws.onmessage = function(event) { try { const data = JSON.parse(event.data); switch (data.type) { case 'sensor': updateSensorReadings(data); break; case 'status': updateStatusInfo(data); break; case 'config_update': loadConfigSettings(data); if (!chart.data.datasets[0].data.length) { updateChart(24); } break; case 'chart_data': processChartData(data); break; case 'mqtt_topic_update': displayStatusMessage(data.message || `MQTT Topic update processed.`, data.success !== false, null, "mqtt-status-msg"); if (data.success && ws && ws.readyState === WebSocket.OPEN) { ws.send(JSON.stringify({ type: 'get_config' })); } break; case 'wifi_update': case 'mqtt_update': case 'calibration_update': case 'temp_unit_changed': case 'reset_initiated': case 'reboot_initiated': displayStatusMessage(data.message || `${data.type} processed.`, data.success !== false); if (data.type === 'mqtt_update' && data.success) { setTimeout(requestStatusUpdate, 1500); } if (data.type === 'calibration_update' && data.success) { if (ws && ws.readyState === WebSocket.OPEN) { ws.send(JSON.stringify({ type: 'get_config' })); } } if (data.type === 'temp_unit_changed') { useCelsius = !data.fahrenheit; updateTempDisplay(); updateChartAxisLabels(); } break; case 'error': console.error("Backend Error:", data.message); displayStatusMessage(`Error: ${data.message}`, false); break; default: console.warn(`Unhandled WebSocket message type: ${data.type}`); } } catch (e) { console.error('Error parsing WebSocket message:', e, event.data); } };
          ws.onclose = function(event) { console.log(`WebSocket disconnected. Code: ${event.code}, Reason: ${event.reason}. Reconnecting...`); updateStatusInfo({ wifi_connected: false, mqtt_connected: false }); configLoaded = false; setTimeout(connectWebSocket, 3000); };
          ws.onerror = function(error) { console.error('WebSocket error:', error); };
      }; // end connectWebSocket

      function updateSensorReadings(data) { document.getElementById('humidity').textContent = parseFloat(data.soil_hum).toFixed(1); currentTempC = parseFloat(data.soil_temp); updateTempDisplay(); document.getElementById('pw_ec').textContent = parseFloat(data.soil_pw_ec).toFixed(2); document.getElementById('ec').textContent = parseFloat(data.soil_ec).toFixed(0); document.getElementById('raw_ec').textContent = parseFloat(data.raw_ec).toFixed(0); const sensorStatus = document.getElementById('sensor-status'); if (data.sensor_error) { sensorStatus.textContent = 'Error'; sensorStatus.className = 'status-badge disconnected'; } else { sensorStatus.textContent = 'OK'; sensorStatus.className = 'status-badge connected'; } }
      function requestStatusUpdate() { if (ws && ws.readyState === WebSocket.OPEN) { console.log("Requesting status update..."); ws.send(JSON.stringify({ type: 'get_status' })); } }
      function updateStatusInfo(data) { const wifiConnected = data.wifi_connected; const wifiStatusElements = [document.getElementById('wifi-status'), document.getElementById('wifi-status-main')]; wifiStatusElements.forEach(el => { if (el) { el.textContent = wifiConnected ? 'Connected' : 'Disconnected'; el.className = `status-badge ${wifiConnected ? 'connected' : 'disconnected'}`; } }); document.getElementById('current-ssid').textContent = wifiConnected ? (data.ssid || '--') : 'N/A'; document.getElementById('current-ip').textContent = wifiConnected ? (data.ip || '--') : 'N/A'; document.getElementById('current-ip-main').textContent = wifiConnected ? (data.ip || '--') : 'N/A'; document.getElementById('current-rssi').textContent = wifiConnected ? (data.rssi || '--') : '--'; const mqttConnected = data.mqtt_connected; const mqttStatusElements = [document.getElementById('mqtt-status'), document.getElementById('mqtt-status-main')]; mqttStatusElements.forEach(el => { if (el) { el.textContent = mqttConnected ? 'Connected' : 'Disconnected'; el.className = `status-badge ${mqttConnected ? 'connected' : 'disconnected'}`; } }); const topic = data.mqtt_topic || '--'; document.getElementById('mqtt-topic-main').textContent = topic; if (document.getElementById('mqtt-topic').value !== topic) { document.getElementById('mqtt-topic-display').textContent = topic; } if (data.uptime !== undefined) { document.getElementById('esp-uptime').textContent = formatUptime(data.uptime); } if (data.mqttServer !== undefined) document.getElementById('mqtt-server-display').textContent = data.mqttServer || '--'; if (data.mqttPort !== undefined) document.getElementById('mqtt-port-display').textContent = data.mqttPort || '--'; if (data.mqttUsername !== undefined) document.getElementById('mqtt-username-display').textContent = data.mqttUsername || '(None)'; }

      function loadConfigSettings(data) {
          console.log("Loading config settings:", data);
          const mqttServerInput = document.getElementById('mqtt-server'); if (!mqttServerInput.value || mqttServerInput.value !== data.mqttServer) mqttServerInput.value = data.mqttServer || '';
          const mqttPortInput = document.getElementById('mqtt-port'); if (!mqttPortInput.value || mqttPortInput.value !== data.mqttPort?.toString()) mqttPortInput.value = data.mqttPort || '';
          const mqttUsernameInput = document.getElementById('mqtt-username'); if (!mqttUsernameInput.value || mqttUsernameInput.value !== data.mqttUsername) mqttUsernameInput.value = data.mqttUsername || '';
          document.getElementById('mqtt-password').value = '';
          const mqttTopicInput = document.getElementById('mqtt-topic'); if (!mqttTopicInput.value || mqttTopicInput.value !== data.mqttTopic) mqttTopicInput.value = data.mqttTopic || '';
          document.getElementById('mqtt-server-display').textContent = data.mqttServer || '--'; document.getElementById('mqtt-port-display').textContent = data.mqttPort || '--'; document.getElementById('mqtt-username-display').textContent = data.mqttUsername || '(None)'; document.getElementById('mqtt-topic-display').textContent = data.mqttTopic || '--';
          // Update Calibration Input fields AND Current Value displays
          document.getElementById('ec-slope').value = parseFloat(data.ecSlope).toFixed(2);
          document.getElementById('ec-intercept').value = parseFloat(data.ecIntercept).toFixed(1);
          document.getElementById('temp-coeff').value = parseFloat(data.ecTempCoeff).toFixed(3);
          document.getElementById('current-ec-slope').textContent = parseFloat(data.ecSlope).toFixed(2); // Update display
          document.getElementById('current-ec-intercept').textContent = parseFloat(data.ecIntercept).toFixed(1); // Update display
          document.getElementById('current-temp-coeff').textContent = parseFloat(data.ecTempCoeff).toFixed(3); // Update display
          useCelsius = !data.useFahrenheit; updateTempDisplay(); updateChartAxisLabels(); configLoaded = true;
      }

      function processChartData(data) { try { const chartDataArray = data.data; if (!Array.isArray(chartDataArray)) { console.error("Chart data received is not an array:", chartDataArray); displayStatusMessage("Failed to load chart data (invalid format).", false, "error-message"); return; } console.log(`Processing ${chartDataArray.length} chart data points.`); if (chartDataArray.length === 0) { console.warn("Received empty chart data array."); } const vwcData = chartDataArray.map(d => ({ x: d.t * 1000, y: parseFloat(d.vwc) })); const pwecData = chartDataArray.map(d => ({ x: d.t * 1000, y: parseFloat(d.pwec) })); const tempData = chartDataArray.map(d => { let tempValueC = parseFloat(d.temp); let displayTemp = useCelsius ? tempValueC : (tempValueC * 9/5) + 32; return { x: d.t * 1000, y: displayTemp }; }); chart.data.datasets[0].data = vwcData; chart.data.datasets[1].data = pwecData; chart.data.datasets[2].data = tempData; updateChartAxisLabels(); chart.update(); console.log("Chart updated."); } catch (e) { console.error("Error processing chart data:", e, data); displayStatusMessage("Error processing chart data.", false, "error-message"); } }
      function updateChartAxisLabels() { if (chart && chart.options && chart.options.scales && chart.options.scales.temp) { chart.options.scales.temp.title.text = `Temperature (${useCelsius ? '°C' : '°F'})`; } }
      function formatUptime(totalSeconds) { if (isNaN(totalSeconds) || totalSeconds < 0) { return "--"; } const days = Math.floor(totalSeconds / (24 * 60 * 60)); let remainingSeconds = totalSeconds % (24 * 60 * 60); const hours = Math.floor(remainingSeconds / (60 * 60)); remainingSeconds %= (60 * 60); const minutes = Math.floor(remainingSeconds / 60); remainingSeconds %= 60; const seconds = Math.floor(remainingSeconds); let result = ''; if (days > 0) result += `${days}d `; if (hours > 0 || days > 0) result += `${hours}h `; if (minutes > 0 || hours > 0 || days > 0) result += `${minutes}m `; result += `${seconds}s`; return result.trim(); }
      function updateTempDisplay() { const tempC = parseFloat(currentTempC); if (isNaN(tempC)) { document.getElementById('temperature').textContent = '--'; document.getElementById('temp-unit').innerHTML = useCelsius ? "&deg;C" : "&deg;F"; return; } const tempValue = useCelsius ? tempC : (tempC * 9/5) + 32; document.getElementById('temperature').textContent = tempValue.toFixed(1); document.getElementById('temp-unit').innerHTML = useCelsius ? "&deg;C" : "&deg;F"; }
      function toggleTempUnit() { useCelsius = !useCelsius; updateTempDisplay(); updateChartAxisLabels(); if (ws && ws.readyState === WebSocket.OPEN) { ws.send(JSON.stringify({ type: 'temp_unit', fahrenheit: !useCelsius })); } if(chart) { chart.update('none'); } }
      function openWebSerial() { window.open('/webserial', '_blank'); }
      function rebootDevice() { document.getElementById('reboot-modal').style.display = "flex"; }
      function confirmReboot(confirm) { document.getElementById('reboot-modal').style.display = "none"; if (confirm && ws && ws.readyState === WebSocket.OPEN) { console.log("Sending reboot command..."); displayStatusMessage("Rebooting device...", true, "info-message"); ws.send(JSON.stringify({type: 'reboot_device'})); } }
      function factoryReset() { document.getElementById('reset-modal').style.display = "flex"; }
      function confirmFactoryReset(confirm) { document.getElementById('reset-modal').style.display = "none"; if (confirm && ws && ws.readyState === WebSocket.OPEN) { console.log("Sending factory reset command..."); displayStatusMessage("Performing factory reset...", true, "info-message"); ws.send(JSON.stringify({type: 'factory_reset'})); } }

      function saveMQTTSettings() { const server = document.getElementById('mqtt-server').value.trim(); const port = document.getElementById('mqtt-port').value.trim(); const username = document.getElementById('mqtt-username').value.trim(); const password = document.getElementById('mqtt-password').value; if (!server || !port) { displayStatusMessage("Server and Port are required for MQTT connection.", false, "error-message", "mqtt-status-msg"); return; } if (isNaN(parseInt(port)) || parseInt(port) <= 0 || parseInt(port) > 65535) { displayStatusMessage("Invalid MQTT Port number.", false, "error-message", "mqtt-status-msg"); return; } document.getElementById('mqtt-values').innerHTML = `<strong>Server:</strong> ${server}<br><strong>Port:</strong> ${port}<br><strong>Username:</strong> ${username || '(None)'}<br><strong>Password:</strong> ${password ? '*'.repeat(Math.min(password.length, 10)) + (password.length > 10 ? '...' : '') : '(None)'}<br>`; document.getElementById('mqtt-modal').style.display = "flex"; }
      function saveMQTTTopic() { const topic = document.getElementById('mqtt-topic').value.trim(); if (!topic) { displayStatusMessage("MQTT Topic cannot be empty.", false, "error-message", "mqtt-status-msg"); return; } if (ws && ws.readyState === WebSocket.OPEN) { console.log("Sending MQTT topic update..."); displayStatusMessage("Saving MQTT topic...", true, "info-message", "mqtt-status-msg"); ws.send(JSON.stringify({ type: 'update_mqtt_topic', topic: topic })); } else { displayStatusMessage("WebSocket not connected. Cannot save topic.", false, "error-message", "mqtt-status-msg"); } }
      function confirmMQTTSettings(confirm) { document.getElementById('mqtt-modal').style.display = "none"; if (confirm && ws && ws.readyState === WebSocket.OPEN) { const server = document.getElementById('mqtt-server').value.trim(); const port = parseInt(document.getElementById('mqtt-port').value.trim()); const username = document.getElementById('mqtt-username').value.trim(); const password = document.getElementById('mqtt-password').value; console.log("Sending MQTT connection settings update..."); displayStatusMessage("Saving MQTT settings...", true, "info-message", "mqtt-status-msg"); ws.send(JSON.stringify({ type: 'update_mqtt', server: server, port: port, user: username, pass: password })); } }
      function saveWiFiSettings() { const ssid = document.getElementById('wifi-ssid').value.trim(); const password = document.getElementById('wifi-password').value; if (!ssid) { displayStatusMessage("WiFi SSID cannot be empty.", false, "error-message", "wifi-status-msg"); return; } if (password.length > 0 && password.length < 8) { displayStatusMessage("WiFi password should be at least 8 characters (or empty for open networks).", false, "error-message", "wifi-status-msg"); } document.getElementById('wifi-values').innerHTML = `<strong>New SSID:</strong> ${ssid}<br><strong>New Password:</strong> ${password ? '*'.repeat(Math.min(password.length, 10)) + (password.length > 10 ? '...' : '') : '(None)'}`; document.getElementById('wifi-modal').style.display = "flex"; }
      function confirmWiFiSettings(confirm) { document.getElementById('wifi-modal').style.display = "none"; if (confirm && ws && ws.readyState === WebSocket.OPEN) { const ssid = document.getElementById('wifi-ssid').value.trim(); const password = document.getElementById('wifi-password').value; console.log("Sending WiFi settings update..."); displayStatusMessage("Saving WiFi settings and rebooting...", true, "info-message", "wifi-status-msg"); ws.send(JSON.stringify({ type: 'update_wifi', ssid: ssid, password: password })); } }
      function saveCalibrationSettings() { const slope = document.getElementById('ec-slope').value.trim(); const intercept = document.getElementById('ec-intercept').value.trim(); const tempCoeff = document.getElementById('temp-coeff').value.trim(); if (slope === '' || intercept === '' || tempCoeff === '') { displayStatusMessage("All calibration fields are required.", false, "error-message", "calibration-status-msg"); return; } if (isNaN(parseFloat(slope)) || isNaN(parseFloat(intercept)) || isNaN(parseFloat(tempCoeff))) { displayStatusMessage("Invalid number entered for calibration.", false, "error-message", "calibration-status-msg"); return; } document.getElementById('calibration-values').innerHTML = `<strong>EC Slope:</strong> ${slope}<br><strong>EC Intercept:</strong> ${intercept}<br><strong>Temp. Coefficient:</strong> ${tempCoeff}`; document.getElementById('calibration-modal').style.display = "flex"; }
      function confirmCalibrationSettings(confirm) { document.getElementById('calibration-modal').style.display = "none"; if (confirm && ws && ws.readyState === WebSocket.OPEN) { const slope = parseFloat(document.getElementById('ec-slope').value.trim()); const intercept = parseFloat(document.getElementById('ec-intercept').value.trim()); const tempCoeff = parseFloat(document.getElementById('temp-coeff').value.trim()); console.log("Sending calibration settings update..."); displayStatusMessage("Saving calibration settings...", true, "info-message", "calibration-status-msg"); ws.send(JSON.stringify({ type: 'update_calibration', slope: slope, intercept: intercept, tempcoeff: tempCoeff })); } }
      function displayStatusMessage(message, isSuccess, className = null, elementId = null) { let statusElement; if (elementId) { statusElement = document.getElementById(elementId); } else { const activeTabId = document.querySelector('.tab.active')?.getAttribute('data-tab'); if (activeTabId) { statusElement = document.getElementById(`${activeTabId.split('-')[0]}-status-msg`); } if (!statusElement) statusElement = document.getElementById('wifi-status-msg'); } if (statusElement) { statusElement.textContent = message; let finalClassName = 'status-message '; if (className) { finalClassName += className; } else { finalClassName += isSuccess ? 'success-message' : 'error-message'; } statusElement.className = finalClassName; setTimeout(() => { if (statusElement.textContent === message) { statusElement.textContent = ''; statusElement.className = 'status-message'; } }, 4000); } else { console.log(`Status (${elementId || 'general'}): ${message} (Success: ${isSuccess})`); } }

      window.onclick = function(event) { const modals = document.getElementsByClassName('modal'); for (let modal of modals) { if (event.target === modal) { modal.style.display = "none"; } } };
      document.addEventListener('DOMContentLoaded', function() { setupTabs(); initChart(); updateClock(); setInterval(updateClock, 1000); connectWebSocket(); });

  </script>
</body>
</html>
)rawliteral";

#endif // HTML_H

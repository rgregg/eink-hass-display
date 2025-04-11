// Fetch current configuration when the page loads
document.addEventListener('DOMContentLoaded', function() {
  fetchConfig();
  
  // Setup form submission
  document.getElementById('configForm').addEventListener('submit', function(e) {
    e.preventDefault();
    saveConfig(false);
  });
  
  // Setup restart button
  document.getElementById('restartBtn').addEventListener('click', function() {
    saveConfig(true);
  });
});

function fetchConfig() {
  fetch('/api/config')
    .then(response => {
      if (!response.ok) {
        throw new Error('Failed to fetch configuration');
      }
      return response.json();
    })
    .then(config => {
      // Populate form fields with current values
      document.getElementById('wifi_ssid').value = config.wifi_ssid || '';
      document.getElementById('wifi_password').value = config.wifi_password || '';
      
      document.getElementById('hass_url').value = config.hass_url || '';
      document.getElementById('hass_token').value = config.hass_token || '';
      
      document.getElementById('weather_entity_id').value = config.weather_entity_id || '';
      document.getElementById('alarm_entity_id').value = config.alarm_entity_id || '';
      document.getElementById('temperature_unit').value = config.temperature_unit || '';
      
      document.getElementById('data_refresh_seconds').value = config.data_refresh_seconds || 300;
      document.getElementById('data_wait_ms').value = config.data_wait_ms || 250;
      
      document.getElementById('listen_for_events').checked = config.listen_for_events || false;
      document.getElementById('wait_for_serial').checked = config.wait_for_serial || false;
      
      // Power management settings
      document.getElementById('enable_deep_sleep').checked = config.enable_deep_sleep || false;
      document.getElementById('sleep_duration_minutes').value = config.sleep_duration_minutes || 5;
      document.getElementById('disable_wifi_between_updates').checked = config.disable_wifi_between_updates || false;
    })
    .catch(error => {
      showStatus('Error loading configuration: ' + error.message, false);
    });
}

function saveConfig(restart) {
  // Collect form data
  const config = {
    wifi_ssid: document.getElementById('wifi_ssid').value,
    wifi_password: document.getElementById('wifi_password').value,
    
    hass_url: document.getElementById('hass_url').value,
    hass_token: document.getElementById('hass_token').value,
    
    weather_entity_id: document.getElementById('weather_entity_id').value,
    alarm_entity_id: document.getElementById('alarm_entity_id').value,
    temperature_unit: document.getElementById('temperature_unit').value,
    
    data_refresh_seconds: parseInt(document.getElementById('data_refresh_seconds').value),
    data_wait_ms: parseInt(document.getElementById('data_wait_ms').value),
    
    listen_for_events: document.getElementById('listen_for_events').checked,
    wait_for_serial: document.getElementById('wait_for_serial').checked,
    
    // Power management settings
    enable_deep_sleep: document.getElementById('enable_deep_sleep').checked,
    sleep_duration_minutes: parseInt(document.getElementById('sleep_duration_minutes').value),
    disable_wifi_between_updates: document.getElementById('disable_wifi_between_updates').checked
  };
  
  // Send to the server
  fetch('/api/config', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
    },
    body: JSON.stringify(config),
  })
  .then(response => response.json())
  .then(data => {
    if (data.status === 'success') {
      showStatus('Configuration saved successfully!', true);
      
      if (restart) {
        showStatus('Restarting device...', true);
        
        // Request device restart
        fetch('/api/restart', {
          method: 'POST'
        })
        .then(() => {
          setTimeout(() => {
            showStatus('Device restarting, please wait about 30 seconds before refreshing...', true);
          }, 1000);
        })
        .catch(error => {
          showStatus('Error restarting: ' + error.message, false);
        });
      }
    } else {
      showStatus('Error: ' + data.message, false);
    }
  })
  .catch(error => {
    showStatus('Error saving configuration: ' + error.message, false);
  });
}

function showStatus(message, isSuccess) {
  const statusElement = document.getElementById('status');
  statusElement.textContent = message;
  statusElement.style.display = 'block';
  
  if (isSuccess) {
    statusElement.className = 'status status-success';
  } else {
    statusElement.className = 'status status-error';
  }
  
  // Auto-hide after 5 seconds
  setTimeout(() => {
    statusElement.style.display = 'none';
  }, 5000);
}
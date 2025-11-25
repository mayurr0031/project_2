// Configuration
const API_URL = ' http://192.168.31.222:5000/api';
const UPDATE_INTERVAL = 2000; // 2 seconds
const CHART_MAX_POINTS = 50;

// Add debug logging
console.log('🚀 Dashboard JavaScript loaded!');
console.log('API URL:', API_URL);

// Chart instances
let powerChart, currentChart, voltageChart;

// Initialize charts
function initCharts() {
    console.log('📊 Initializing charts...');
    
    const commonOptions = {
        responsive: true,
        maintainAspectRatio: false,
        plugins: {
            legend: {
                display: true,
                position: 'top'
            }
        },
        scales: {
            x: {
                display: true,
                title: {
                    display: true,
                    text: 'Time'
                }
            },
            y: {
                display: true,
                beginAtZero: true
            }
        }
    };

    // Power Chart
    const powerCtx = document.getElementById('powerChart').getContext('2d');
    powerChart = new Chart(powerCtx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [
                {
                    label: 'Power 1 (W)',
                    data: [],
                    borderColor: 'rgb(255, 99, 132)',
                    backgroundColor: 'rgba(255, 99, 132, 0.1)',
                    tension: 0.4
                },
                {
                    label: 'Power 2 (W)',
                    data: [],
                    borderColor: 'rgb(54, 162, 235)',
                    backgroundColor: 'rgba(54, 162, 235, 0.1)',
                    tension: 0.4
                },
                {
                    label: 'Total Power (W)',
                    data: [],
                    borderColor: 'rgb(75, 192, 192)',
                    backgroundColor: 'rgba(75, 192, 192, 0.1)',
                    tension: 0.4
                }
            ]
        },
        options: commonOptions
    });

    // Current Chart
    const currentCtx = document.getElementById('currentChart').getContext('2d');
    currentChart = new Chart(currentCtx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [
                {
                    label: 'Current 1 (A)',
                    data: [],
                    borderColor: 'rgb(255, 159, 64)',
                    backgroundColor: 'rgba(255, 159, 64, 0.1)',
                    tension: 0.4
                },
                {
                    label: 'Current 2 (A)',
                    data: [],
                    borderColor: 'rgb(153, 102, 255)',
                    backgroundColor: 'rgba(153, 102, 255, 0.1)',
                    tension: 0.4
                },
                {
                    label: 'Total Current (A)',
                    data: [],
                    borderColor: 'rgb(201, 203, 207)',
                    backgroundColor: 'rgba(201, 203, 207, 0.1)',
                    tension: 0.4
                }
            ]
        },
        options: commonOptions
    });

    // Voltage Chart
    const voltageCtx = document.getElementById('voltageChart').getContext('2d');
    voltageChart = new Chart(voltageCtx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [
                {
                    label: 'Voltage (V)',
                    data: [],
                    borderColor: 'rgb(255, 205, 86)',
                    backgroundColor: 'rgba(255, 205, 86, 0.1)',
                    tension: 0.4
                }
            ]
        },
        options: commonOptions
    });
    
    console.log('✅ Charts initialized successfully');
}

// Update charts with new data
function updateCharts(timestamp, data) {
    const timeLabel = new Date(timestamp).toLocaleTimeString();

    // Update Power Chart
    powerChart.data.labels.push(timeLabel);
    powerChart.data.datasets[0].data.push(data.power1);
    powerChart.data.datasets[1].data.push(data.power2);
    powerChart.data.datasets[2].data.push(data.total_power);

    // Update Current Chart
    currentChart.data.labels.push(timeLabel);
    currentChart.data.datasets[0].data.push(data.current1);
    currentChart.data.datasets[1].data.push(data.current2);
    currentChart.data.datasets[2].data.push(data.total_current);

    // Voltage Chart
    voltageChart.data.labels.push(timeLabel);
    voltageChart.data.datasets[0].data.push(data.voltage);

    // Limit chart data points
    if (powerChart.data.labels.length > CHART_MAX_POINTS) {
        powerChart.data.labels.shift();
        powerChart.data.datasets.forEach(dataset => dataset.data.shift());
        
        currentChart.data.labels.shift();
        currentChart.data.datasets.forEach(dataset => dataset.data.shift());
        
        voltageChart.data.labels.shift();
        voltageChart.data.datasets[0].data.shift();
    }

    // Update charts
    powerChart.update('none');
    currentChart.update('none');
    voltageChart.update('none');
}

// Fetch latest data from server
async function fetchLatestData() {
    try {
        console.log('📡 Fetching latest data...');
        const response = await fetch(`${API_URL}/latest`);
        
        console.log('Response status:', response.status);
        
        if (response.ok) {
            const data = await response.json();
            console.log('✅ Data received:', data);
            updateDashboard(data);
            updateConnectionStatus(true);
        } else {
            console.error('❌ Server returned error:', response.status);
            updateConnectionStatus(false);
        }
    } catch (error) {
        console.error('❌ Error fetching data:', error);
        updateConnectionStatus(false);
    }
}

// Fetch statistics
async function fetchStatistics() {
    try {
        const response = await fetch(`${API_URL}/stats?period=day`);
        
        if (response.ok) {
            const data = await response.json();
            console.log('📊 Stats received:', data);
            updateStatistics(data);
        }
    } catch (error) {
        console.error('❌ Error fetching statistics:', error);
    }
}

// Update dashboard with new data
function updateDashboard(data) {
    console.log('🔄 Updating dashboard...');
    
    // Update real-time values
    document.getElementById('voltage').textContent = data.voltage.toFixed(2) + ' V';
    document.getElementById('current1').textContent = data.current1.toFixed(3) + ' A';
    document.getElementById('current2').textContent = data.current2.toFixed(3) + ' A';
    document.getElementById('current3').textContent = data.current3.toFixed(3) + ' A';
    document.getElementById('totalCurrent').textContent = data.total_current.toFixed(3) + ' A';
    
    document.getElementById('power1').textContent = data.power1.toFixed(2) + ' W';
    document.getElementById('power2').textContent = data.power2.toFixed(2) + ' W';
    document.getElementById('totalPower').textContent = data.total_power.toFixed(2) + ' W';
    document.getElementById('totalPowerHeader').textContent = data.total_power.toFixed(1) + ' W';
    
    const apparentPower = data.voltage * data.total_current;
    document.getElementById('apparentPower').textContent = apparentPower.toFixed(2) + ' VA';
    
    // Update relay states (from server)
    document.getElementById('relay1').checked = data.relay1_state;
    document.getElementById('relay2').checked = data.relay2_state;
    
    // Update last update time
    const lastUpdate = new Date(data.timestamp);
    document.getElementById('lastUpdate').textContent = lastUpdate.toLocaleTimeString();
    
    // Update charts
    updateCharts(data.timestamp, data);
    
    console.log('✅ Dashboard updated successfully');
}

// Update statistics
function updateStatistics(data) {
    if (data.avg_voltage) {
        document.getElementById('avgVoltage').textContent = data.avg_voltage.toFixed(1) + 'V';
    }
    if (data.avg_current) {
        document.getElementById('avgCurrent').textContent = data.avg_current.toFixed(2) + 'A';
    }
    if (data.max_power) {
        document.getElementById('maxPower').textContent = data.max_power.toFixed(1) + 'W';
    }
    if (data.total_energy_kwh) {
        document.getElementById('totalEnergy').textContent = data.total_energy_kwh.toFixed(2) + 'kWh';
    } else {
        document.getElementById('totalEnergy').textContent = '--';
    }
}

// Update connection status
function updateConnectionStatus(isConnected) {
    const statusElement = document.getElementById('connectionStatus');
    
    if (isConnected) {
        statusElement.textContent = 'Online';
        statusElement.className = 'status-value online';
    } else {
        statusElement.textContent = 'Offline';
        statusElement.className = 'status-value offline';
    }
}

// Toggle relay - UPDATED to use new endpoint
async function toggleRelay(relay, state) {
    try {
        console.log(`🔌 Toggling Relay ${relay} to ${state ? 'ON' : 'OFF'}`);
        
        const response = await fetch(`${API_URL}/relay/control`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                relay: relay,
                state: state
            })
        });
        
        if (response.ok) {
            const result = await response.json();
            console.log('✅ Relay command sent:', result);
            
            // Keep the user's selection
            // ESP32 will sync with server within 3 seconds
        } else {
            console.error('❌ Failed to toggle relay:', response.status);
            // Revert button on error
            const checkbox = document.getElementById(`relay${relay}`);
            checkbox.checked = !state;
        }
    } catch (error) {
        console.error('❌ Error toggling relay:', error);
        // Revert button on error
        const checkbox = document.getElementById(`relay${relay}`);
        checkbox.checked = !state;
    }
}

// Switch tabs
function switchTab(tabName) {
    // Remove active class from all tabs and content
    document.querySelectorAll('.tab').forEach(tab => tab.classList.remove('active'));
    document.querySelectorAll('.tab-content').forEach(content => content.classList.remove('active'));
    
    // Add active class to selected tab
    event.target.classList.add('active');
    document.getElementById(tabName + 'Tab').classList.add('active');
}

// Load historical data
async function loadHistoricalData() {
    try {
        console.log('📈 Loading historical data...');
        const response = await fetch(`${API_URL}/history?hours=24`);
        
        if (response.ok) {
            const data = await response.json();
            console.log(`✅ Loaded ${data.length} historical records`);
            
            // Clear existing chart data
            powerChart.data.labels = [];
            powerChart.data.datasets.forEach(dataset => dataset.data = []);
            currentChart.data.labels = [];
            currentChart.data.datasets.forEach(dataset => dataset.data = []);
            voltageChart.data.labels = [];
            voltageChart.data.datasets[0].data = [];
            
            // Add historical data to charts
            data.forEach(reading => {
                const timeLabel = new Date(reading.timestamp).toLocaleTimeString();
                
                powerChart.data.labels.push(timeLabel);
                powerChart.data.datasets[0].data.push(reading.power1);
                powerChart.data.datasets[1].data.push(reading.power2);
                powerChart.data.datasets[2].data.push(reading.total_power);
                
                currentChart.data.labels.push(timeLabel);
                currentChart.data.datasets[0].data.push(reading.current1);
                currentChart.data.datasets[1].data.push(reading.current2);
                currentChart.data.datasets[2].data.push(reading.total_current);
                
                voltageChart.data.labels.push(timeLabel);
                voltageChart.data.datasets[0].data.push(reading.voltage);
            });
            
            // Update charts
            powerChart.update();
            currentChart.update();
            voltageChart.update();
        }
    } catch (error) {
        console.error('❌ Error loading historical data:', error);
    }
}

// Initialize application
function init() {
    console.log('⚡ Initializing Energy Meter Dashboard...');
    
    // Initialize charts
    initCharts();
    
    // Load historical data
    loadHistoricalData();
    
    // Fetch initial data
    fetchLatestData();
    fetchStatistics();
    
    // Set up periodic updates
    setInterval(fetchLatestData, UPDATE_INTERVAL);
    setInterval(fetchStatistics, 60000); // Update stats every minute
    
    console.log('✅ Dashboard initialized successfully');
    console.log(`🔄 Updating every ${UPDATE_INTERVAL/1000} seconds`);
}

// Start application when DOM is ready
document.addEventListener('DOMContentLoaded', init);
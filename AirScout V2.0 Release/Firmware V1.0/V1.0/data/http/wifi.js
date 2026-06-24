let table = document.getElementById('scan_wifi_body');
let scanNetworksTable = document.getElementById('scan_wifi_body');
let knownNetworksTable = document.getElementById('known_networks_body')
let connectedNetworkTable = document.getElementById('connected_wifi_body')
const passwdDialog = document.getElementById('passwdDialog');
const confirmBtn = passwdDialog.querySelector('#confirmBtn');
const cancelBtn = passwdDialog.querySelector('#cancelBtn');
const inputPasswd = document.getElementById('passwdInput');
const passwdDialogLabel = document.getElementById('passwdDialogLabel');
const passwdDialogSSIDField = document.getElementById('passwdDialogSSIDField');
const passwdForm = document.getElementById('passwdForm');
let lastConnected = '', knownNetworks = [];
let networkToAdd = {};
const passwdLabelText = passwdDialogLabel.textContent;
let getWifiDataIntervalID;
let wifiData;
let askForAllData = true;

window.onload = function() {
    passwdDialog.addEventListener('close', (e) => {
        console.log('Closing Form');
    });


    confirmBtn.addEventListener('click', (event) => {
        event.preventDefault();
        console.log('Confirm')

        console.log(passwdDialogSSIDField.value)
        console.log(inputPasswd.value)
        if (inputPasswd.value.length < 8) {
            alert(strings.passwdTooShort);
        }
        else {
            sendGet({'type': 'add', 'ssid': passwdDialogSSIDField.value, 'passwd': inputPasswd.value})
            passwdDialog.close();
            alert("Closing AP.. Restart WifiSetup to reach this site again");
        }
    });


    cancelBtn.addEventListener('click', (event) => {
        inputPasswd.value = '';
        console.log('Cancel')
        event.preventDefault()
        passwdDialog.close();
    })
    passwdForm.addEventListener('submit', (e) => {
        console.log('SUBMITTTT');
    });
    passwdForm.reset();
    getSavedNets();
    getWifiDataIntervalID = setInterval(getWifiData, 6000);
    getWifiData();
};

async function getSavedNets() {
    const response = await fetch(`http://${window.location.hostname}/stored_nets`);
    if (!response.ok) {
        return
    }
    updatePage(await response.json());
}

async function getWifiData() {
    const response = await fetch(`http://${window.location.hostname}/wifidata`);
    if (!response.ok) {
        return
    }

    updatePage(await response.json());
}

function showPassword() {
    checkBox = document.getElementById('showPasswordInputCheckBox');
    if (checkBox.checked) {
        passwdInput.type = 'text';

    } else {
        passwdInput.type = 'password';
    }
}

function updatePage(data) {
    console.log(data);

    if (Object.keys(data).includes('known')) {
        knownNetworks = data.known;
        addKnownNetworksToTable(data.known);

    } else if (Object.keys(data).includes('update')) {
        updateTable(data);
    } else if (Object.keys(data).includes('status')) {
        updateStatus(data);
    }
}

function sortScanNets(data) {
    var networks = []
    networks = data.update;
    var availableKnownNetworks = networks.filter(function(net) {
        for (knownNet of knownNetworks) {
            if (knownNet.ssid == net.ssid && knownNet.ssid != data.connected) {
                return true
            }
        }
        return false
    })

    availableKnownNetworks.sort(function(a, b) {
        return parseInt(b.rssi) - parseInt(a.rssi);
    });

    var availableUnknownNetworks = networks.filter(function(net) {
        for (knownNet of knownNetworks) {
            if (knownNet.ssid == net.ssid) {
                return false
            }
        }
        return true
    })

    availableUnknownNetworks.sort(function(a, b) {
        return parseInt(b.rssi) - parseInt(a.rssi);
    });

    return {'known': availableKnownNetworks, 'unknown': availableUnknownNetworks};
}

function updateStatus(data) {
    switch (data.status) {
        case 'connection_successful':
            console.log('connection_successful')
            break;
        case 'connection_failed':
            console.log('connection_failed')
            break;
        case 'disconnection_successful':
            console.log('disconnection_successful')
            break;
        case 'delete_successful':
            console.log('delete_successful')
            break;
        case 'delete_failed':
            console.log('connection_failed')
            break;
    }
}

function addAllScanNetworksToTable(nets) {
    scanNetworksTable.innerHTML = '';
    availableKnownNetworks = nets.known;
    availableUnknownNetworks = nets.unknown;
    addScanNetworksToTable(availableKnownNetworks, 'known');
    addScanNetworksToTable(availableUnknownNetworks, 'unknown');
}

function updateTable(data) {
    addConnectedNetworkToTable(data)
    nets = sortScanNets(data)
    addAllScanNetworksToTable(nets)
}

function addScanNetworksToTable(networks, type) {
    var newCell;
    for (let i = 0; i < networks.length; i++) {
        var cell = 0
        var newRow = scanNetworksTable.insertRow();

        newCell = newRow.insertCell(cell++);
        insertSVG(newCell, networks[i]);

        newCell = newRow.insertCell(cell++);
        insertSSID(newCell, networks[i]);

        known = false;
        for (knownNet of knownNetworks) {
            if (networks[i].ssid == knownNet.ssid) {
                known = true;
            }
        }
        newCell = newRow.insertCell(cell++);
        if (type == 'known') {
            insertButton(newCell, () => connectToKnownWifi(networks[i]), strings.connectButton, 'button button_connect');
        } else {
            insertButton(newCell, () => addWifiCbk(networks[i]), strings.addNetworkButton, 'button button_connect');
        }
    }
}

function addConnectedNetworkToTable(data) {
    var newCell;
    connectedNetworkTable.innerHTML = '';
    var cell = 0
    var network = {};

    for (scan of data.update) {
        if (scan.ssid == data.connected) {
            network = scan;
        }
    }

    if (!Object.keys(network).includes('ssid')) {
        return
    }

    var newRow = connectedNetworkTable.insertRow();
    newCell = newRow.insertCell(cell++);
    insertSVG(newCell, network);

    newCell = newRow.insertCell(cell++);
    insertSSID(newCell, network);

    newCell = newRow.insertCell(cell++);
    insertButton(newCell, () => disconnectWifi(network), strings.disconnectButton, additionalClass = 'button_disconnect')
}

function addKnownNetworksToTable(networks) {
    knownNetworksTable.innerHTML = '';

    for (let i = 0; i < networks.length; i++) {
        var cell = 0
        var newRow = knownNetworksTable.insertRow();
        newCell = newRow.insertCell(cell++);
        insertSVG(newCell, networks[i]);
        newCell = newRow.insertCell(cell++);
        insertSSID(newCell, networks[i], colspan = 1);
        newCell = newRow.insertCell(cell++);
        insertButton(newCell, () => deleteWifi(networks[i]), strings.deleteButton, additionalClass = 'button_disconnect');
    }
}

function insertSVG(cell, net) {
    // -90dbm = 0%, -30dBm is 100% signal Quality
    if (Object.keys(net).includes('rssi')) {
        var quality = Math.min(Math.max(Math.round(150 - (5 / 3) * Math.abs(net.rssi)), 0), 100);
    } else {
        var quality = -100;
    }

    if (quality > 80) {
        svgSignalStrength = '100';
    } else if (quality > 55) {
        svgSignalStrength = '80';
    } else if (quality > 30) {
        svgSignalStrength = '60';
    } else if (quality >= 0) {
        svgSignalStrength = '40';
    }

    if (Object.keys(net).includes('security')) {
        if (net.security == 0) {
            svgSignalSecurity = '';
        } else {
            svgSignalSecurity = '-locked';
        }
    } else {
        svgSignalSecurity = '';
    }

    if (quality > 0) {
        svgPath = `icons/network-wireless-${svgSignalStrength}${svgSignalSecurity}.svg`;
    } else {
        svgPath = `icons/network-wireless-disconnected.svg`;
    }

    var img = document.createElement('img');
    img.setAttribute('src', svgPath);
    img.setAttribute('class', 'wifi_symbol');
    cell.appendChild(img);
}

function insertSSID(cell, net, colspan = 1) {
    if (colspan > 1) {
        cell.setAttribute('colspan', colspan);
    }
    cell.appendChild(document.createTextNode(net.ssid));
}

function insertButton(cell, callback, text, additionalClass = '') {
    let button = document.createElement('button');
    button.setAttribute('class', `button ${additionalClass}`);
    button.appendChild(document.createTextNode(text));
    button.addEventListener('click', callback);
    cell.appendChild(button);
}

function deleteWifi(network) {
    console.log('Deleting', network)
    sendGet({'type': 'delete', 'ssid': network.ssid});
    setTimeout(getSavedNets, 1000);
}

function connectToKnownWifi(network) {
    console.log('Connecting to', network);
    sendGet({'type': 'connect', 'ssid': network.ssid});
    alert("Closing AP.. Restart WifiSetup to reach this site again")
}

function addWifiCbk(network) {
    networkToAdd = network;
    if (network.security == 0) {
        sendGet({'type': 'add', 'ssid': network.ssid})
        alert("Closing AP.. Restart WifiSetup to reach this site again");
        // addWifi(network)
    } else {
        passwdDialogLabel.textContent = passwdLabelText + ' ' + network.ssid;
        passwdDialog.showModal()
        inputPasswd.value = '';
        passwdDialogSSIDField.value = network.ssid;
    }
}

function disconnectWifi(network) {
    sendGet({'type': 'disconnect', 'ssid': network.ssid})
    console.log('Disconnting', network);
}

async function sendGet(data, url = 'settings') {
    const response = await fetch(`http://${window.location.hostname}/${url}?` + encodeQueryData(data))

    if (!response.ok) {
        console.log('Failed to send Settings')
    }
    data = {}
}
// https://stackoverflow.com/a/111545
function encodeQueryData(data) {
    console.log('Encoding Data', data);
    let ret = [];

    for (let [key, value] of Object.entries(data)) {
        ret.push(encodeURIComponent(key) + '=' + encodeURIComponent(value));
        console.log(key + ':' + value);
    }
    return ret.join('&');
}


/*

    var request = new XMLHttpRequest();
    request.open('GET', url, true);
    request.onload = function() { // request successful
    // we can use server response to our request now
    console.log(request.responseText);
    };


    request.onerror = function() {
    // request failed
    };

    request.send(data); // create FormData from form that triggered event

}
*/

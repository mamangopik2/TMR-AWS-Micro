

function setCookie(cname, cvalue, exdays) {
    const d = new Date();
    d.setTime(d.getTime() + (exdays * 24 * 60 * 60 * 1000));
    let expires = "expires=" + d.toUTCString();
    document.cookie = cname + "=" + encodeURIComponent(cvalue) + ";" + expires + ";path=/";
}

function getCookie(cname) {
    let name = cname + "=";
    let ca = document.cookie.split(';');
    for (let i = 0; i < ca.length; i++) {
        let c = ca[i].trim();
        if (c.indexOf(name) == 0) {
            return decodeURIComponent(c.substring(name.length, c.length));
        }
    }
    return "";
}


console.log("saved cookies", getCookie("login"));
if (getCookie("login") != "true") {
    if (location.pathname != "/login.html") {
        window.location = window.location.origin + '/login.html';
    }
}


document.getElementById("dropdown-buttons").innerHTML =
    `
    <button id="button_LOGOUT">LOGOUT</button>
    <button id="device_info">DEVICE INFO</button>
    <button id="button_configured_sensor">CONFIGURED SENSOR</button>
    <button id="button_network">NETWORK</button>
    <button id="button_interfaces">MODBUS MASTER</button>
    <button id="button_sensors">ADD SENSORS</button>
    <button id="button_site_setup">SITE SETUP</button>
    <button id="button_time_setup">TIME SETUP</button>
    <button id="button_cloud_setup">CLOUD SETUP</button>
    <button id="button_OTA">FIRMWARE UPDATE</button>
    `;


// Navigation
document.getElementById("button_network").onclick = () => window.location = base_url + '/net-form.html';
document.getElementById("button_sensors").onclick = () => window.location = base_url + '/sensors.html';
document.getElementById("button_interfaces").onclick = () => window.location = base_url + '/interfaces.html';
document.getElementById("button_site_setup").onclick = () => window.location = base_url + '/site_setup.html';
document.getElementById("button_time_setup").onclick = () => window.location = base_url + '/time_setup.html';
document.getElementById("button_cloud_setup").onclick = () => window.location = base_url + '/cloud_setup.html';
document.getElementById("button_OTA").onclick = () => window.location = base_url + '/OTA.html';
document.getElementById("button_configured_sensor").onclick = () => window.location = base_url + '/data_viewer.html';
document.getElementById("device_info").onclick = () => window.location = base_url + '/device-info.html';
document.getElementById("button_LOGOUT").onclick = () => {
    setCookie("login", "", 3600);
    window.location.reload();
}
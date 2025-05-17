function fetchBeacon() {
    const baseUrl = `${window.location.protocol}//${window.location.host}`;
    const endpoint = `${baseUrl}/beacon`;

    fetch(endpoint, {
        method: 'GET',
        headers: {
            'Accept': 'application/json'
        }
    })
        .then(response => {
            if (!response.ok) {
                throw new Error(`HTTP error! Status: ${response.status}`);
            }
            return response.json();
        })
        .then(data => {
            console.log("Response from /beacon:", data);
        })
        .catch(error => {
            console.error("Error fetching /beacon:", error);
        });
}

// Run immediately
fetchBeacon();

// Then run every 10 seconds
setInterval(fetchBeacon, 10000);

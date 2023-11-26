function sendApiRequest() {
    // Get the content of the textbox
    var textBoxContent = document.getElementById("textbox").value;

    // Make a fetch request
    fetch("/api/search?content=" + encodeURIComponent(textBoxContent))
        .then(response => {
            if (!response.ok) {
                throw new Error("Error: Unable to make API request.");
            }
            return response.text();
        })
        .then(data => {
            // Extract the content from the response
            var contentStart = data.indexOf("Content: ");
            if (contentStart !== -1) {
                var content = data.substring(contentStart + "Content: ".length);
                // Display the response in the designated area
                document.getElementById("responseArea").innerHTML = content; 
     } else {
                // Display an error message in case of an unexpected response
                document.getElementById("responseArea").innerHTML = "Error: Unexpected response from the server.";
            }
        })
        .catch(error => {
            // Display an error message in case of a network error or other issues
            document.getElementById("responseArea").innerHTML = error.message;
        });
}


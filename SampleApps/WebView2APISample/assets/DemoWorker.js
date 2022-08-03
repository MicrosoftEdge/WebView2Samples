onconnect = function (e) {
    var port = e.ports[0];

    port.onmessage = function (e) {
        var workerResult = 'Local script result: ' + e.data[0] + " * " + e.data[1] + " = " + (e.data[0] * e.data[1]);
        port.postMessage(workerResult);
    }
}

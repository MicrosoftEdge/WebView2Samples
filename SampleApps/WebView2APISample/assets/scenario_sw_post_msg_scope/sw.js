'use strict';

if(self.chrome.webview) {
  self.chrome.webview.addEventListener('message', event => {
    if(event.data === 'PostMessageToSW') {
      self.chrome.webview.postMessage("SWDirectMessage");
    }
  });

}

self.addEventListener('message', event => {
  if(event.data === 'MainThreadMessage') {
    console.log(event.data)
    clients.matchAll().then((clients) => {
        clients.forEach((client) => {
            client.postMessage(event.data);
        });
    });
  }
})
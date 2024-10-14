function fetchAndCacheLatestNews() {
    console.log("Fetched news from a server");
  }
  
  self.addEventListener("periodicsync", (event) => {
    console.log("Periodic Sync Task Tag: " + event.tag + " executed");
    event.waitUntil(fetchAndCacheLatestNews());
  });
  
  self.addEventListener("sync", (event) => {
    console.log("Background Sync Task Tag: " + event.tag + " executed");
  });
'use strict';

const CACHE_NAME = 'sw_post_message_cache';
const CACHE_LIST = ['style.css'];

const cacheFirst = async (request) => {
  // First try to get the resource from the cache
  const responseFromCache = await caches.match(request);
  if (responseFromCache) {
    console.log('Cache hit for request: ', request.url);
    // Notify the app about the cache hit.
    //! [chromeWebView]
    self.chrome.webview.postMessage('Cache hit for resource: ' + request.url);
    //! [chromeWebView]
    return responseFromCache;
  }

  // Next try to get the resource from the network
  try {
    console.log('Cache miss for request: ', request.url);
    const responseFromNetwork = await fetch(request);
    const cache = await caches.open(CACHE_NAME);
    console.log('Cache new resource: ', request.url);
    await cache.put(request, responseFromNetwork.clone());
    return responseFromNetwork;
  } catch (error) {
    return new Response('Network error happened', {
      status: 408,
      headers: { 'Content-Type': 'text/plain' },
    });
  }
};

const addToCache = async (url) => {
  console.log('Add to cache: ', url);
  const cache = await caches.open(CACHE_NAME);
  cache.add(url);
  //! [chromeWebView]
  self.chrome.webview.postMessage('Added to cache: ' + url);
  //! [chromeWebView]
};

self.addEventListener('install', (event) => {
  event.waitUntil(
    caches
      .open(CACHE_NAME)
      .then((cache) => cache.addAll(CACHE_LIST))
      .then(self.skipWaiting())
  );
});

self.addEventListener('activate', (event) => {
  event.waitUntil(clients.claim());
});

self.addEventListener('fetch', (event) => {
  event.respondWith(cacheFirst(event.request));
});

//! [chromeWebView]
self.chrome.webview.addEventListener('message', (e) => {
  if (e.data.command === 'ADD_TO_CACHE') {
    addToCache(e.data.url);
  }
});
//! [chromeWebView]

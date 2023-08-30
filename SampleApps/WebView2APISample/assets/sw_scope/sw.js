'use strict';

const CACHE_NAME = 'sw_cache';
const CACHE_LIST = [
  'cached_by_sw_install.jpg'
];

self.addEventListener('install', event => {
    console.log('sw_scope/sw.js handles install event');
    event.waitUntil(caches.open(CACHE_NAME)
      .then(cache => cache.addAll(CACHE_LIST))
      .then(self.skipWaiting())
    );
});

self.addEventListener('activate', event => {
    console.log('sw_scope/sw.js handles activate event');
    event.waitUntil(clients.claim());
});

self.addEventListener('fetch', event => {
    console.log('sw_scope/sw.js handles fetch event for', event.request.url);
    if (event.request.url.indexOf('txt_from_local_asset') != -1) {
        event.respondWith(fetch('./simple.txt'));
    } else if (event.request.url.indexOf('image_from_cache') != -1) {
        event.respondWith(caches.match('./cached_by_sw_install.jpg'));
    } else {
        event.respondWith(fetch(event.request));
    }
});
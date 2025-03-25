// target number of iterations
const limitIterations = false;
const iterations = 20;

// steps per iteration
const steps = 80;
const target = steps + 1;

// global state
let iteration = 0;
let counter = 0;
let first = performance.now();
let timerId = undefined;

const frameId = document.querySelector('meta[name="frame-title"]').content;
const logger = (frameId == 'main') ? window.open('/ScenarioThrottlingControlMonitor.html') : window.parent;

const timerCallback = () => {
  if (++counter == 1) {
    first = performance.now();
  }

  // compute average timer delay only after target steps
  if (counter == target) {
    let end = performance.now();
    let avg = (end - first) / steps;
    onIterationCompleted(avg);
  }
}

function reportAverageDelay(delay) {
  console.log(`[${frameId}] avg: ${delay} ms`);
  let message = {
    "frameId": frameId,
    "delayAvg": delay
  };
  logger.postMessage(message);
}

function onIterationCompleted(delayAvg) {
  counter = 0;

  if (++iteration == iterations && limitIterations) {
    clearInterval(timerId);
    timerId = undefined;
  }

  reportAverageDelay(delayAvg);
}

document.addEventListener("visibilitychange", () => {
  let message = {
    "frameId": frameId,
    "visibilityUpdate": document.visibilityState
  };
  logger.postMessage(message);
});

window.addEventListener('message', (event) => {
  console.log(`[${frameId}] got message: ${event.data}`);

  if (event.data == 'init') {
    // init timer/state
    counter = 0;
    start = performance.now();
    timerId = setInterval(timerCallback, 0);

    // fwd to embedded frames
    if (frameId == 'main') {
      document.getElementById('trusted').contentWindow.postMessage(event.data);
      document.getElementById('untrusted').contentWindow.postMessage(event.data);
    }
  } else if (frameId == 'main') {
    // log from embedded frame, fwd to popup
    logger.postMessage(event.data);
  }
});

let counters = {
  'main': 0,
  'trusted': 0,
  'untrusted': 0
};

function logLine (target, content) {
  let output = document.getElementById(`output-${target}`);
  let line = `${counters[`${target}`]++}`;
  output.textContent += `${line.padStart(3, ' ')} | ${content}\n`;

  output.scrollTop = output.scrollHeight;
}

window.addEventListener('message', (event) => {
  console.log(event.data);

  let frameId = event.data.frameId;

  if (event.data.visibilityUpdate) {
    let visibility = event.data.visibilityUpdate;
    logLine(frameId, `[visibility: ${visibility}]`);
    document.getElementById('page-state').textContent = visibility == 'hidden' ? 'background' : 'foreground';
  } else {
    // reporting delay
    let delayText = event.data.delayAvg.toFixed(2);
    logLine(frameId, `${delayText} ms`);
  }
});

function toggleVisibility() {
  let message = {
    command: 'toggle-visibility',
  };

  chrome.webview.postMessage(message);
}

function setTimerInterval(priority) {
  let interval = document.getElementById(`interval-${priority}`).value;
  if (interval === '' || isNaN(interval)) {
    console.log('invalid value');
    return;
  }

  let message = {
    command: 'set-interval',
    params: {
      priority: priority,
      intervalMs: interval
    }
  };

  console.log(message);
  chrome.webview.postMessage(message);
}

function triggerScenario(label) {
  let message = {
    command: 'scenario',
    params: {
      label: label
    }
  };

  chrome.webview.postMessage(message);
}

// notify target so it can start timers
window.opener.postMessage('init');

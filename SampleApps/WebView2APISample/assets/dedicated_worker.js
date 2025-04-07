//! [chromeWebView]
self.chrome.webview.addEventListener('message', (e) => {
  const data = e.data;
  if (!data.hasOwnProperty('first') || !data.hasOwnProperty('second') ||
      !data.hasOwnProperty('command')) {
    return;
  }

  const first = data.first;
  const second = data.second;
  switch (data.command) {
    case 'ADD': {
      result = first + second;
      break;
    }
    case 'SUB': {
      result = first - second;
      break;
    }
    case 'MUL': {
      result = first * second;
      break;
    }
    case 'DIV': {
      if (second === 0) {
        result = 'Error: Division by zero';
        break;
      }

      result = first / second;
      break;
    }
    default: {
      result = 'Failed to process the command';
    }
  }

  // Notify the app about the result.
  self.chrome.webview.postMessage('Result = ' + result.toString());
});
//! [chromeWebView]

// Sensitivity Label API Sample
// This sample demonstrates how to use the PageInteractionRestrictionManager API
// to add and remove sensitivity labels in WebView2

// Element ID constants
const ELEMENT_IDS = {
  OUTPUT: 'output',
  LABEL_ID: 'labelId',
  ORG_ID: 'orgId',
  LABEL_SELECT: 'labelSelect',
  PIRM_STATUS: 'pirmStatus',
  PIRM_STATUS_TEXT: 'pirmStatusText'
};

let labelMap = new Map(); // Store label objects for removal

function log(message, is_error = false) {
  const output = document.getElementById(ELEMENT_IDS.OUTPUT);

  // Simply update the text content and color
  output.textContent = message;
  output.style.color = is_error ? 'red' : '';

  console.log(message);
}

async function addLabel() {
  const labelId = document.getElementById(ELEMENT_IDS.LABEL_ID).value.trim();
  const orgId = document.getElementById(ELEMENT_IDS.ORG_ID).value.trim();

  if (!labelId || !orgId) {
      log('Error: Please enter both Label ID and Organization ID', true /* is_error */);
      return;
  }

  if (labelMap.has(labelId)) {
      log(`Error: Label ${labelId} already exists`, true /* is_error */);
      return;
  }

  try {
      log(`Adding sensitivity label: ${labelId}`);

      // Request label manager from PageInteractionRestrictionManager API
      const labelManager = await navigator.pageInteractionRestrictionManager.requestLabelManager();

      // Add the sensitivity label
      const label = await labelManager.addLabel('MicrosoftSensitivityLabel', {
          label: labelId,
          organization: orgId
      });

      // Store label object for removal later
      labelMap.set(labelId, label);

      log(`Success: Added sensitivity label ${labelId}`);
      addLabelToDropdown(labelId);
      clearInputs();

  } catch (error) {
      log(`Error adding label: ${error.message}`, true /* is_error */);
  }
}

async function removeLabel() {
  const selectedLabelId = document.getElementById(ELEMENT_IDS.LABEL_SELECT).value;

  const labelObject = labelMap.get(selectedLabelId);
  if (!labelObject) {
      log('Error: Label object not found', true /* is_error */);
      return;
  }

  try {
      log(`Removing sensitivity label: ${selectedLabelId}`);

      // Remove the sensitivity label
      await labelObject.remove();

      // Remove from our map
      labelMap.delete(selectedLabelId);

      log(`Success: Removed sensitivity label ${selectedLabelId}`);
      removeLabelFromDropdown(selectedLabelId);

  } catch (error) {
      log(`Error removing label: ${error.message}`, true /* is_error */);
  }
}

function addLabelToDropdown(labelId) {
  const select = document.getElementById(ELEMENT_IDS.LABEL_SELECT);

  // If this is the first label, clear the "No labels available" option
  if (select.options.length === 1 && select.options[0].value === '') {
    select.options[0].textContent = 'Select a label to remove';
  }

  // Add the newly added label
  const option = document.createElement('option');
  option.value = labelId;
  option.textContent = labelId;

  select.appendChild(option);
}

function removeLabelFromDropdown(labelId) {
  const select = document.getElementById(ELEMENT_IDS.LABEL_SELECT);
  const options = select.options;

  for (let i = 0; i < options.length; i++) {
    if (options[i].value === labelId) {
        select.removeChild(options[i]);
        break;
    }
  }

  // If no labels left, add "No labels available" option
  if (select.options.length === 1 && select.options[0].value === '') {
    select.options[0].textContent = 'No labels available';
  }
}

function clearInputs() {
  document.getElementById(ELEMENT_IDS.LABEL_ID).value = '';
  document.getElementById(ELEMENT_IDS.ORG_ID).value = '';
}

function checkPirmAvailability() {
  const statusBox = document.getElementById(ELEMENT_IDS.PIRM_STATUS);
  const statusText = document.getElementById(ELEMENT_IDS.PIRM_STATUS_TEXT);

  try {
      const isAvailable = typeof navigator.pageInteractionRestrictionManager !== 'undefined' &&
                          navigator.pageInteractionRestrictionManager !== null;

      if (isAvailable) {
          statusBox.className = 'status-box available';
          statusText.textContent = '✅ PageInteractionRestrictionManager API is available and ready to use!';
          log('PIRM API available');
      } else {
          statusBox.className = 'status-box unavailable';
          statusText.textContent = '❌ PageInteractionRestrictionManager API is not available. Please check if the feature is enabled and the URL is added to the allowlist.';
          log('PIRM API not available');
      }
  } catch (error) {
      statusBox.className = 'status-box unavailable';
      statusText.textContent = '❌ Error checking API availability: ' + error.message;
      log('Error checking PIRM availability: ' + error.message, true /* is_error */);
  }
}

// Initialize with sample values
document.addEventListener('DOMContentLoaded', function() {
  // Set initial status
  const statusBox = document.getElementById(ELEMENT_IDS.PIRM_STATUS);
  statusBox.className = 'status-box checking';

  document.getElementById(ELEMENT_IDS.LABEL_ID).value = '12345678-1234-1234-1234-123456789abc';
  document.getElementById(ELEMENT_IDS.ORG_ID).value = '87654321-4321-4321-4321-cba987654321';

  log('Sensitivity Label API sample loaded');
  checkPirmAvailability();
});

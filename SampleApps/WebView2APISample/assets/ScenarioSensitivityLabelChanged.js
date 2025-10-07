console.log('Sensitivity Label Changed scenario page loaded');
console.log('Event handler is now active and waiting for sensitivity label changes');

// Element ID constants
const ELEMENT_IDS = {
    PIRM_WARNING_BOX: 'pirmWarningBox',
    PIRM_AVAILABLE_BOX: 'pirmAvailableBox',
    CURRENT_URL: 'currentUrl',
    ADD_LABEL_BTN: 'addLabelBtn',
    REMOVE_LABEL_BTN: 'removeLabelBtn',
    REMOVE_LABEL_SELECT: 'removeLabelSelect',
    LABEL_ID_INPUT: 'labelIdInput',
    ORG_ID_INPUT: 'orgIdInput',
    RESULT_BOX: 'resultBox',
    ERROR_BOX: 'errorBox',
    RESULT_MESSAGE: 'resultMessage',
    ERROR_MESSAGE: 'errorMessage'
};

let isPirmAvailable = false;
let labelMap = new Map(); // Map of label ID to label object

// Hide previous result and error messages
function hideResultMessages() {
    const resultBox = document.getElementById(ELEMENT_IDS.RESULT_BOX);
    const errorBox = document.getElementById(ELEMENT_IDS.ERROR_BOX);

    if (resultBox) resultBox.style.display = 'none';
    if (errorBox) errorBox.style.display = 'none';
}

// Show error message to user
function showErrorMessage(message) {
    const errorBox = document.getElementById(ELEMENT_IDS.ERROR_BOX);
    const errorMessage = document.getElementById(ELEMENT_IDS.ERROR_MESSAGE);

    if (errorMessage) errorMessage.textContent = message;
    if (errorBox) errorBox.style.display = 'block';
}

// Show success message to user
function showSuccessMessage(message) {
    const resultBox = document.getElementById(ELEMENT_IDS.RESULT_BOX);
    const resultMessage = document.getElementById(ELEMENT_IDS.RESULT_MESSAGE);

    if (resultMessage) resultMessage.textContent = message;
    if (resultBox) resultBox.style.display = 'block';
}

// Get input values for sensitivity label
function getLabelInputs() {
    const labelId = document.getElementById(ELEMENT_IDS.LABEL_ID_INPUT)?.value.trim() || '';
    const orgId = document.getElementById(ELEMENT_IDS.ORG_ID_INPUT)?.value.trim() || '';
    return { labelId, orgId };
}

// Clear input fields
function clearInputFields() {
    const labelIdInput = document.getElementById(ELEMENT_IDS.LABEL_ID_INPUT);
    const orgIdInput = document.getElementById(ELEMENT_IDS.ORG_ID_INPUT);

    if (labelIdInput) labelIdInput.value = '';
    if (orgIdInput) orgIdInput.value = '';
}

// Set button state (enabled/disabled with loading text)
function setButtonState(elementId, disabled, loadingText = null) {
    const button = document.getElementById(elementId);
    if (!button) return;

    button.disabled = disabled;
    if (loadingText && disabled) {
        button.textContent = loadingText;
    } else if (elementId === ELEMENT_IDS.ADD_LABEL_BTN) {
        button.textContent = 'Add Sensitivity Label';
    } else if (elementId === ELEMENT_IDS.REMOVE_LABEL_BTN) {
        button.textContent = 'Remove Selected Label';
    }
}

// Update button visual states based on availability and label count
function updateButtonStates() {
    const addLabelBtn = document.getElementById(ELEMENT_IDS.ADD_LABEL_BTN);
    const removeLabelBtn = document.getElementById(ELEMENT_IDS.REMOVE_LABEL_BTN);

    if (addLabelBtn) {
        addLabelBtn.disabled = !isPirmAvailable;
        addLabelBtn.style.opacity = isPirmAvailable ? '1' : '0.5';
    }
    if (removeLabelBtn) {
        removeLabelBtn.disabled = !isPirmAvailable || labelMap.size === 0;
        removeLabelBtn.style.opacity = (!isPirmAvailable || labelMap.size === 0) ? '0.5' : '1';
    }
}

// Check PIRM availability on page load
function checkPirmAvailability() {
    console.log('Checking PageInteractionRestrictionManager availability...');

    try {
        isPirmAvailable = typeof navigator.pageInteractionRestrictionManager !== 'undefined' &&
                        navigator.pageInteractionRestrictionManager !== null;

        console.log('PIRM Available:', isPirmAvailable);
        updatePirmBoxVisibility();
    } catch (error) {
        console.error('Error checking PIRM availability:', error);
        isPirmAvailable = false;
        updatePirmBoxVisibility();
    }
}

// Update visibility of PIRM status boxes
function updatePirmBoxes() {
    const pirmWarningBox = document.getElementById(ELEMENT_IDS.PIRM_WARNING_BOX);
    const pirmAvailableBox = document.getElementById(ELEMENT_IDS.PIRM_AVAILABLE_BOX);
    const currentUrlElement = document.getElementById(ELEMENT_IDS.CURRENT_URL);

    if (isPirmAvailable) {
        console.log('Setting pirmAvailableBox to visible');
        if (pirmWarningBox) pirmWarningBox.style.display = 'none';
        if (pirmAvailableBox) pirmAvailableBox.style.display = 'block';
    } else {
        console.log('Setting pirmWarningBox to visible');
        if (pirmWarningBox) pirmWarningBox.style.display = 'block';
        if (pirmAvailableBox) pirmAvailableBox.style.display = 'none';

        // Show current URL when not available
        if (currentUrlElement) {
            currentUrlElement.textContent = window.location.href;
        }
    }
}

// Update PIRM box visibility based on availability
function updatePirmBoxVisibility() {
    updatePirmBoxes();
    updateButtonStates();
    updateRemoveLabelDropdown();
}

// Clear all options from a select element
function clearSelectOptions(selectElement) {
    while (selectElement.firstChild) {
        selectElement.removeChild(selectElement.firstChild);
    }
}

// Create and return an option element
function createOption(value, text) {
    const option = document.createElement('option');
    option.value = value;
    option.textContent = text;
    return option;
}

// Populate dropdown with label options
function populateLabelDropdown(selectElement) {
    if (labelMap.size === 0) {
        selectElement.appendChild(createOption('', 'No labels available'));
        selectElement.disabled = true;
    } else {
        selectElement.appendChild(createOption('', 'Select a label to remove'));

        for (const [labelId] of labelMap) {
            selectElement.appendChild(createOption(labelId, labelId));
        }

        selectElement.disabled = false;
    }
}

// Update the remove label dropdown with current labels
function updateRemoveLabelDropdown() {
    const removeLabelSelect = document.getElementById(ELEMENT_IDS.REMOVE_LABEL_SELECT);
    if (!removeLabelSelect) return;

    clearSelectOptions(removeLabelSelect);
    populateLabelDropdown(removeLabelSelect);
}

// Validate inputs for adding sensitivity label
function validateAddLabelInputs(labelId, orgId) {
    if (!labelId || !orgId) {
        return 'Please enter both Label ID GUID and Organization ID GUID.';
    }

    if (labelMap.has(labelId)) {
        return `Label ID ${labelId} already exists. Please use a different Label ID or remove the existing one first.`;
    }

    return null; // No validation errors
}

// Add sensitivity label via API
async function addSensitivityLabelAPI(labelId, orgId) {
    if (!navigator.pageInteractionRestrictionManager) {
        throw new Error('pageInteractionRestrictionManager API not available. This may require adding the page to the allowlist.');
    }

    const lm = await navigator.pageInteractionRestrictionManager.requestLabelManager();
    const label = await lm.addLabel('MicrosoftSensitivityLabel', {
        label: labelId,
        organization: orgId
    });

    return label;
}

async function applySensitivityLabel() {
    const { labelId, orgId } = getLabelInputs();

    hideResultMessages();

    // Validate inputs
    const validationError = validateAddLabelInputs(labelId, orgId);
    if (validationError) {
        showErrorMessage(validationError);
        return;
    }

    setButtonState(ELEMENT_IDS.ADD_LABEL_BTN, true, 'Adding Label...');

    try {
        console.log('Attempting to apply sensitivity label...');
        console.log('Label ID:', labelId);
        console.log('Organization ID:', orgId);

        const label = await addSensitivityLabelAPI(labelId, orgId);

        // Store the label object for later removal
        labelMap.set(labelId, label);

        console.log('Sensitivity label applied successfully:', label);
        console.log('Current label map:', labelMap);

        showSuccessMessage(`Sensitivity label added successfully! Label ID: ${labelId}, Organization ID: ${orgId}`);
        updateRemoveLabelDropdown();
        clearInputFields();

    } catch (error) {
        console.error('Error applying sensitivity label:', error);
        showErrorMessage(`Failed to add sensitivity label: ${error.message}`);
    } finally {
      setButtonState(ELEMENT_IDS.ADD_LABEL_BTN, !isPirmAvailable);
      setButtonState(ELEMENT_IDS.REMOVE_LABEL_BTN,
          !isPirmAvailable || labelMap.size === 0);
    }
}

// Get selected label ID from dropdown
function getSelectedLabelId() {
    const removeLabelSelect = document.getElementById(ELEMENT_IDS.REMOVE_LABEL_SELECT);
    return removeLabelSelect?.value || '';
}

async function removeSensitivityLabel() {
    hideResultMessages();

    const selectedLabelId = getSelectedLabelId();

    // Validate inputs
    if (!selectedLabelId) {
        showErrorMessage('Please select a label ID to remove.');
        return;
    }

    const labelObject = labelMap.get(selectedLabelId);

    setButtonState(ELEMENT_IDS.REMOVE_LABEL_BTN, true, 'Removing Label...');

    try {
        console.log('Attempting to remove sensitivity label...');
        console.log('Selected Label ID:', selectedLabelId);
        console.log('Label object:', labelObject);

        const result = await labelObject.remove();

        // Remove the label from the map since it's been removed
        labelMap.delete(selectedLabelId);

        console.log('Sensitivity label removed successfully:', result);
        console.log('Updated label map:', labelMap);

        showSuccessMessage(
            `Sensitivity label removed successfully! Label ID: ${selectedLabelId}`
        );
        updateRemoveLabelDropdown();
    } catch (error) {
        console.error('Error removing sensitivity label:', error);
        showErrorMessage(
            `Failed to remove sensitivity label: ${error.message}`
        );
    } finally {
        setButtonState(
            ELEMENT_IDS.REMOVE_LABEL_BTN,
            !isPirmAvailable || labelMap.size === 0
        );
    }
}

// Pre-fill input fields with sample GUIDs for testing
function preFillSampleInputs() {
    const labelIdInput = document.getElementById(ELEMENT_IDS.LABEL_ID_INPUT);
    const orgIdInput = document.getElementById(ELEMENT_IDS.ORG_ID_INPUT);

    if (labelIdInput && orgIdInput) {
        labelIdInput.value = '12345678-1234-1234-1234-123456789abc';
        orgIdInput.value = '87654321-4321-4321-4321-cba987654321';
        console.log('Pre-filled input values');
    } else {
        console.error('Could not find input elements');
    }
}

// Initialize the application
function initializeApp() {
    console.log('Initializing sensitivity label app...');
    preFillSampleInputs();
    checkPirmAvailability();
}

// Setup event listeners and initialization
function setupApplication() {
    console.log('Setting up application...');

    if (document.readyState === 'loading') {
        console.log('DOM still loading, waiting for DOMContentLoaded');
        document.addEventListener('DOMContentLoaded', initializeApp);
    } else {
        console.log('DOM already ready, initializing immediately');
        initializeApp();
    }
}

// Start the application
console.log('Sensitivity Label Changed scenario page loaded');
console.log('Event handler is now active and waiting for sensitivity label changes');
setupApplication();

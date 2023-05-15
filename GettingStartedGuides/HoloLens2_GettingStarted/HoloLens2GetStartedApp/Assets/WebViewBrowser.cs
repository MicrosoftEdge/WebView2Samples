using Microsoft.MixedReality.WebView;
using UnityEngine.UI;
using UnityEngine;
using TMPro;
using System;

public class WebViewBrowser : MonoBehaviour
{
    // Declare UI elements: back button, go button, and URL input field
    public Button BackButton;
    public Button GoButton;
    public TMP_InputField URLField;

    private void Start()
    {
        // Get the WebView component attached to the game object
        var webViewComponent = gameObject.GetComponent<WebView>();
        webViewComponent.GetWebViewWhenReady((IWebView webView) =>
        {
            // If the WebView supports browser history, enable the back button
            if (webView is IWithBrowserHistory history)
            {
                // Add an event listener for the back button to navigate back in history
                BackButton.onClick.AddListener(() => history.GoBack());

                // Update the back button's enabled state based on whether there's any history to go back to
                history.CanGoBackUpdated += CanGoBack;
            }

            // Add an event listener for the go button to load the URL entered in the input field
            GoButton.onClick.AddListener(() => webView.Load(new Uri(URLField.text)));

            // Subscribe to the Navigated event to update the URL input field whenever a navigation occurs
            webView.Navigated += OnNavigated;

            // Set the initial value of the URL input field to the current URL of the WebView
            if (webView.Page != null)
            {
                URLField.text = webView.Page.AbsoluteUri;
            }
        });
    }

    // Update the URL input field with the new path after navigation
    private void OnNavigated(string path)
    {
        URLField.text = path;
    }

    // Enable or disable the back button based on whether there's any history to go back to
    private void CanGoBack(bool value)
    {
        BackButton.enabled = value;
    }
}
using Microsoft.MixedReality.WebView;
using UnityEngine.UI;
using UnityEngine;
using TMPro;
using System;

using Microsoft.MixedReality.Toolkit.Input;

public class WebViewBrowser : MonoBehaviour, IMixedRealityPointerHandler
{
    // Declare UI elements: back button, go button, and URL input field
    public Button BackButton;
    public Button GoButton;
    public TMP_InputField URLField;
    public MeshCollider Collider;
    private IWebView _webView;

    private void Start()
    {
        // Get the WebView component attached to the game object
        var webViewComponent = gameObject.GetComponent<WebView>();
        webViewComponent.GetWebViewWhenReady((IWebView webView) =>
        {
            _webView = webView;

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

    public void OnPointerDown(MixedRealityPointerEventData eventData)
    {
        TranslateToWebViewMouseEvent(eventData, WebViewMouseEventData.EventType.MouseDown);
    }

    public void OnPointerUp(MixedRealityPointerEventData eventData)
    {
        TranslateToWebViewMouseEvent(eventData, WebViewMouseEventData.EventType.MouseUp);
    }

    private void TranslateToWebViewMouseEvent(MixedRealityPointerEventData eventData, WebViewMouseEventData.EventType evenType)
    {
        var hitCoord = NormalizeWorldPoint(eventData.Pointer.Result.Details.Point);

        hitCoord.x *= _webView.Width;
        hitCoord.y *= _webView.Height;

        var mouseEventsWebView = _webView as IWithMouseEvents;
        WebViewMouseEventData mouseEvent = new WebViewMouseEventData
        {
            X = (int)hitCoord.x,
            Y = (int)hitCoord.y,
            Device = WebViewMouseEventData.DeviceType.Pointer,
            Type = evenType,
            Button = WebViewMouseEventData.MouseButton.ButtonLeft,
            TertiaryAxisDeviceType = WebViewMouseEventData.TertiaryAxisDevice.PointingDevice
        };

        mouseEventsWebView.MouseEvent(mouseEvent);
    }

    private Vector2 NormalizeWorldPoint(Vector3 worldPoint)
    {
        // Convert the world point to our control's local space.
        Vector3 localPoint = transform.InverseTransformPoint(worldPoint);

        var boundsSize = Collider.sharedMesh.bounds.size;
        var boundsExtents = Collider.sharedMesh.bounds.max;

        // Adjust the point to be based on a 0,0 origin.
        var uvTouchPoint = new Vector2((localPoint.x + boundsExtents.x), -1.0f * (localPoint.y - boundsExtents.y));

        // Normalize the point so it can be mapped to the WebView's texture.
        var normalizedPoint = new Vector2(uvTouchPoint.x / boundsSize.x, uvTouchPoint.y / boundsSize.y);

        return normalizedPoint;
    }

    public void OnPointerClicked(MixedRealityPointerEventData eventData) { }
    public void OnPointerDragged(MixedRealityPointerEventData eventData) { }
}
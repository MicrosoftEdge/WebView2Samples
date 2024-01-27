using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;
using System;
using Microsoft.Web.WebView2.Core;
using System.Diagnostics;
// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace WebView2_WinUI3_Sample
{
    /// <summary>
    /// An empty window that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainWindow : Window
    {
        public MainWindow()
        {
            this.InitializeComponent();
            AddressBar.Text = "https://developer.microsoft.com/en-us/microsoft-edge/webview2/";

            WebView2.NavigationCompleted += WebView2_NavigationCompleted;
            WebView2.CoreWebView2Initialized += WebView2_CoreWebView2Initialized;

            WebView2.Source = new Uri(AddressBar.Text);
            StatusUpdate("Ready");
            SetTitle();
        }

        private void StatusUpdate(string message)
        {
            StatusBar.Text = message;
            Debug.WriteLine(message);
        }

        private void WebView2_CoreWebView2Initialized(WebView2 sender, CoreWebView2InitializedEventArgs args)
        {
            if (args.Exception != null)
            {
                StatusUpdate($"Error initializing WebView2: {args.Exception.Message}");
            }
            else
            {
                SetTitle(sender);
            }
        }

        private void WebView2_NavigationCompleted(WebView2 sender, CoreWebView2NavigationCompletedEventArgs args)
        {
            StatusUpdate("Navigation complete");

            // Update the address bar with the full URL that was navigated to.
            AddressBar.Text = sender.Source.ToString();
        }

        private bool TryCreateUri(String potentialUri, out Uri result)
        {
            StatusUpdate("TryCreateUri");

            Uri uri;
            if ((Uri.TryCreate(potentialUri, UriKind.Absolute, out uri) || Uri.TryCreate("http://" + potentialUri, UriKind.Absolute, out uri)) &&
                (uri.Scheme == Uri.UriSchemeHttp || uri.Scheme == Uri.UriSchemeHttps))
            {
                result = uri;
                return true;
            }
            else
            {
                StatusUpdate("Unable to configure URI");
                result = null;
                return false;
            }
        }

        private void TryNavigate()
        {
            StatusUpdate("TryNavigate");

            Uri destinationUri;
            if (TryCreateUri(AddressBar.Text, out destinationUri))
            {
                WebView2.Source = destinationUri;
            }
            else
            {
                StatusUpdate("URI couldn't be figured out use it as a bing search term");

                String bingString = $"https://www.bing.com/search?q={Uri.EscapeDataString(AddressBar.Text)}";
                if (TryCreateUri(bingString, out destinationUri))
                {
                    AddressBar.Text = destinationUri.AbsoluteUri;
                    WebView2.Source = destinationUri;
                }
                else
                {
                    StatusUpdate("URI couldn't be configured as bing search term, giving up");
                }
            }
        }

        private void Go_OnClick(object sender, RoutedEventArgs e)
        {
            StatusUpdate("Go_OnClick: " + AddressBar.Text);

            TryNavigate();
        }

        private void AddressBar_KeyDown(object sender, KeyRoutedEventArgs e)
        {
            if (e.Key == Windows.System.VirtualKey.Enter)
            {
                StatusUpdate("AddressBar_KeyDown [Enter]: " + AddressBar.Text);

                e.Handled = true;
                TryNavigate();
            }
        }

        private void SetTitle(WebView2 webView2 = null)
        {
            var packageDisplayName = Windows.ApplicationModel.Package.Current.DisplayName;
            var webView2Version = (webView2 != null) ? " - " + GetWebView2Version(webView2) : string.Empty;
            Title = $"{packageDisplayName}{webView2Version}";
        }

        private string GetWebView2Version(WebView2 webView2)
        {
            var runtimeVersion = webView2.CoreWebView2.Environment.BrowserVersionString;

            CoreWebView2EnvironmentOptions options = new CoreWebView2EnvironmentOptions();
            var targetVersionMajorAndRest = options.TargetCompatibleBrowserVersion;
            var versionList = targetVersionMajorAndRest.Split('.');
            if (versionList.Length != 4)
            {
                return "Invalid SDK build version";
            }
            var sdkVersion = versionList[2] + "." + versionList[3];

            return $"{runtimeVersion}; {sdkVersion}";
        }
    }
}

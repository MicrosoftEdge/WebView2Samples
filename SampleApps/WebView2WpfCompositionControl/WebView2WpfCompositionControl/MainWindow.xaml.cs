using System.Windows;
using Microsoft.Web.WebView2.Core;

namespace WebView2WpfCompositionControl
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();

            webView.CoreWebView2InitializationCompleted += WebView_CoreWebView2InitializationCompleted;
            webView.NavigationStarting += WebView_NavigationStarting;
            webView.NavigationCompleted += WebView_NavigationCompleted;
            webView.WebMessageReceived += WebView_WebMessageReceived;
            webView.Source = new Uri("https://www.bing.com");
        }

        private void WebView_CoreWebView2InitializationCompleted(object? sender, CoreWebView2InitializationCompletedEventArgs e)
        {
            status.Content = "Initialized:" + (e.InitializationException?.ToString() ?? "Success");
        }

        private void WebView_WebMessageReceived(object? sender, CoreWebView2WebMessageReceivedEventArgs e)
        {
            status.Content = e.TryGetWebMessageAsString();
        }

        private void WebView_NavigationStarting(object? sender, CoreWebView2NavigationStartingEventArgs e)
        {
            status.Content = "NavigationStarting";
        }

        private void WebView_NavigationCompleted(object? sender, CoreWebView2NavigationCompletedEventArgs args)
        {
            // Update address bar
            addressBar.Text = webView.Source.ToString();
            string info = (args.IsSuccess ? webView.CoreWebView2.DocumentTitle : "error");
            status.Content = $"NavigationCompleted:{info}";
        }

        private void GoButton_Click(object sender, RoutedEventArgs e)
        {
            if (webView != null && webView.CoreWebView2 != null)
            {
                String uri = addressBar.Text;
                if (uri.StartsWith("http://")) { uri.Replace("http://", "https://"); }
                try
                {
                    webView.CoreWebView2.Navigate(uri);
                }
                catch
                {
                    status.Content = "InvalidUrl";
                }
            }
        }

        private void ToggleButton_Click(object sender, RoutedEventArgs e)
        {
            airspaceButton.Visibility = (airspaceButton.IsVisible ? Visibility.Collapsed : Visibility.Visible);
            status.Content = "Toggle:" + airspaceButton.Visibility;
        }

        private void AirspaceButton_Click(object sender, RoutedEventArgs e)
        {
            status.Content = "AirspaceClicked";
        }

        private void AddressBar_GotFocus(object sender, RoutedEventArgs e)
        {
            status.Content = "GotFocus:AddressBar";
        }

        private void GoButton_GotFocus(object sender, RoutedEventArgs e)
        {
            status.Content = "GotFocus:GoButton";
        }

        private void ToggleButton_GotFocus(object sender, RoutedEventArgs e)
        {
            status.Content = "GotFocus:ToggleButton";
        }

        private void AirspaceButton_GotFocus(object sender, RoutedEventArgs e)
        {
            status.Content = "GotFocus:AirspaceButton";
        }
    }

}


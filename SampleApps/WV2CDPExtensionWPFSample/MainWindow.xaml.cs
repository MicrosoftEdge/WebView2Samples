using System;
using System.Diagnostics;
using System.Windows;
using System.Windows.Input;
using Microsoft.Web.WebView2.Core;
using Microsoft.Web.WebView2.Wpf;
using Microsoft.Web.WebView2.Core.DevToolsProtocolExtension;
using System.Text;
using System.Linq;

namespace WV2CDPExtensionSample
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        bool _isNavigating = false;
        public static RoutedCommand CallCDPMethodCommand = new RoutedCommand();
        DevToolsProtocolHelper _cdpHelper;
        DevToolsProtocolHelper cdpHelper
        {
            get
            {
                if (webView == null || webView.CoreWebView2 == null)
                {
                    throw new Exception("Initialize WebView before using DevToolsProtocolHelper.");
                }

                if (_cdpHelper == null)
                {
                    _cdpHelper = webView.CoreWebView2.GetDevToolsProtocolHelper();
                }

                return _cdpHelper;
            }
        }
        public MainWindow()
        {
            InitializeComponent();
        }

        void GoToPageCmdCanExecute(object sender, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = webView != null && !_isNavigating;
        }

        async void GoToPageCmdExecuted(object target, ExecutedRoutedEventArgs e)
        {
            await webView.EnsureCoreWebView2Async();
            webView.CoreWebView2.Navigate((string)e.Parameter);
        }


        #region CDP_COMMANDS
        async void ShowFPSCounter(object sender, RoutedEventArgs e)
        {
            await cdpHelper.Overlay.SetShowFPSCounterAsync(true);
        }

        async void HideFPSCounter(object sender, RoutedEventArgs e)
        {
            await cdpHelper.Overlay.SetShowFPSCounterAsync(false);
        }

        async void SetPageScaleTo4(object sender, RoutedEventArgs e)
        {
            await cdpHelper.Emulation.SetPageScaleFactorAsync(4);
        }
       
        async void ResetPageScale(object sender, RoutedEventArgs e)
        {
            await cdpHelper.Emulation.SetPageScaleFactorAsync(1);
        }

        async void ReloadPage(object sender, RoutedEventArgs e)
        {
            await cdpHelper.Page.ReloadAsync();
        }

        async void CaptureSnapshot(object sender, RoutedEventArgs e)
        {
            Trace.WriteLine(await cdpHelper.Page.CaptureSnapshotAsync());
        }

        async void GetAllCookies(object sender, RoutedEventArgs e) 
        {
            Network.Cookie[] cookies = await cdpHelper.Network.GetAllCookiesAsync();
            StringBuilder cookieResult = new StringBuilder(cookies.Count() + " cookie(s) received\n");
            foreach (var cookie in cookies)
            {
                cookieResult.Append($"\n{cookie.Name} {cookie.Value} {(cookie.Session ? "[session cookie]" : cookie.Expires.ToString("G"))}");
            }
            MessageBox.Show(cookieResult.ToString(), "Cookies");
        }

        async void AddOrUpdateCookie(object target, RoutedEventArgs e)
        {
            bool cookie = await cdpHelper.Network.SetCookieAsync("CookieName", "CookieValue", null, "https://www.bing.com/");
            MessageBox.Show(cookie ? "Cookie is added/updated successfully" : "Error adding/updating cookie", "Cookies");
        }

        async void ClearAllCookies(object sender, RoutedEventArgs e)
        {
            await cdpHelper.Network.ClearBrowserCookiesAsync();
            MessageBox.Show("Browser cookies are deleted", "Cookies");
        }

        async void SetGeolocation(object sender, RoutedEventArgs e)
        {
            double latitude = 36.553085;
            double longitude = 103.97543;
            double accuracy = 1;
            await cdpHelper.Emulation.SetGeolocationOverrideAsync(latitude, longitude, accuracy);
            MessageBox.Show("Overridden the Geolocation Position", "Geolocation");
        }

        async void ClearGeolocation(object sender, RoutedEventArgs e)
        {
            await cdpHelper.Emulation.ClearGeolocationOverrideAsync();
            MessageBox.Show("Cleared overridden Geolocation Position", "Geolocation");
        }
        #endregion

        #region CDP_EVENTS
        async void SubscribeToDataReceived(object sender, RoutedEventArgs e)
        {
            await cdpHelper.Network.EnableAsync();
            cdpHelper.Network.DataReceived += PrintDataReceived;
            MessageBox.Show("Subscribed to DataReceived Event!", "DataReceived");
        }

        void UnsubscribeFromDataReceived(object sender, RoutedEventArgs e)
        {
            cdpHelper.Network.DataReceived -= PrintDataReceived;
            MessageBox.Show("Unsubscribed from DataReceived Event!", "DataReceived");
        }

        void PrintDataReceived(object sender, Network.DataReceivedEventArgs args)
        {
            Trace.WriteLine(String.Format("DataReceived Event Args - Timestamp: {0}   Request Id: {1}   DataLength: {2}", args.Timestamp, args.RequestId, args.DataLength));
        }

        async void SubscribeToAnimationCreated(object sender, RoutedEventArgs e)
        {
            await cdpHelper.Animation.EnableAsync();
            cdpHelper.Animation.AnimationCreated += PrintAnimationCreated;
            MessageBox.Show("Subscribed to AnimationCreated Event!", "AnimationCreated");
        }

        void UnsubscribeFromAnimationCreated(object sender, RoutedEventArgs e)
        {
            cdpHelper.Animation.AnimationCreated -= PrintAnimationCreated;
            MessageBox.Show("Unsubscribed from AnimationCreated Event!", "AnimationCreated");
        }

        void PrintAnimationCreated(object sender, Animation.AnimationCreatedEventArgs args)
        {
            Trace.WriteLine(String.Format("AnimationCreated Event Args - Id: {0}", args.Id));
        }

        async void SubscribeToDocumentUpdated(object sender, RoutedEventArgs e)
        {
            await cdpHelper.DOM.EnableAsync();
            cdpHelper.DOM.DocumentUpdated += PrintDocumentUpdated;
            MessageBox.Show("Subscribed to DocumentUpdated Event!", "DocumentUpdated");
        }

        void UnsubscribeFromDocumentUpdated(object sender, RoutedEventArgs e)
        {
            cdpHelper.DOM.DocumentUpdated -= PrintDocumentUpdated;
            MessageBox.Show("Unsubscribed from DocumentUpdated Event!", "DocumentUpdated");
        }

        void PrintDocumentUpdated(object sender, DOM.DocumentUpdatedEventArgs args)
        {
            Trace.WriteLine("DocumentUpdated Event Args - No Parameters", "DocumentUpdated");
        }

        async void SubscribeToDownloadWillBegin(object sender, RoutedEventArgs e)
        {
            await cdpHelper.Page.EnableAsync();
            cdpHelper.Page.DownloadWillBegin += PrintDownloadWillBegin;
            MessageBox.Show("Subscribed to DownloadWillBegin Event!", "DownloadWillBegin");
        }

        void UnsubscribeFromDownloadWillBegin(object sender, RoutedEventArgs e)
        {
            cdpHelper.Page.DownloadWillBegin -= PrintDownloadWillBegin;
            MessageBox.Show("Unsubscribed from DownloadWillBegin Event!", "DownloadWillBegin");
        }

        void PrintDownloadWillBegin(object sender, Page.DownloadWillBeginEventArgs args)
        {
            Trace.WriteLine(String.Format("DownloadWillBegin Event Args - FrameId: {0}   Guid: {1}   URL: {2}", args.FrameId, args.Guid, args.Url));
        }
        #endregion
    }
}

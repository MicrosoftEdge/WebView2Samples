using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;
using ABI.Microsoft.UI.Private.Controls;

// The Blank Window item template is documented at https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

namespace WinUI_Sample
{
    /// <summary>
    /// An empty window that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainWindow : Window
    {
        public MainWindow()
        {
            this.InitializeComponent();
            MyWebView.NavigationStarting += EnsureHttps;
            //MyWebView.WebMessageReceived += UpdateAddressBar;
            
        }

        private void UpdateAddressBar(WebView2 sender, WebView2WebMessageReceivedEventArgs args)
        {
            //String uri = args.Source;
            //addressBar.Text = uri;
        }

        private void EnsureHttps(WebView2 sender, WebView2NavigationStartingEventArgs args)
        {
            String uri = args.Uri;
            if (!uri.StartsWith("https://"))
            {
                MyWebView.ExecuteScriptAsync($"alert('{uri} is not safe, try an https link')");
                args.Cancel = true;
            }
            else
            {
                addressBar.Text = uri;
            }
        }


private void myButton_Click(object sender, RoutedEventArgs e)
        {
            //myButton.Content = "Clicked";

            try
            {
                Uri targetUri = new Uri(addressBar.Text);
                MyWebView.Source = targetUri;
            }
            catch (FormatException ex)
            {
                // Bad address.
            }

        }
    }
}

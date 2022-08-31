using System;
using System.Collections.Generic;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;
using Microsoft.Web.WebView2.Core;

namespace WebView2WpfBrowser
{
    /// <summary>
    /// Interaction logic for Extensions.xaml
    /// </summary>
    public partial class Extensions : Window
    {
        private CoreWebView2 m_coreWebView2;
        public Extensions(CoreWebView2 coreWebView2)
        {
            m_coreWebView2 = coreWebView2;
            InitializeComponent();
            _ = FillViewAsync();
        }

        public class ListEntry
        {
            public string Name;
            public string Id;
            public bool Enabled;

            public override string ToString()
            {
                return (Enabled ? "" : "Disabled ") + Name + " (" + Id + ")";
            }
        }

        List<ListEntry> m_listData = new List<ListEntry>();

        private async System.Threading.Tasks.Task FillViewAsync()
        {
        }
        private void ExtensionsToggleEnabled(object sender, RoutedEventArgs e)
        {
            _ = ExtensionsToggleEnabledAsync(sender, e);
        }

        private async System.Threading.Tasks.Task ExtensionsToggleEnabledAsync(object sender, RoutedEventArgs e)
        {
            await FillViewAsync();
        }
        private void ExtensionsAdd(object sender, RoutedEventArgs e)
        {
            _ = ExtensionsAddAsync(sender, e);
        }

        private async System.Threading.Tasks.Task ExtensionsAddAsync(object sender, RoutedEventArgs e)
        {
        }
        private void ExtensionsRemove(object sender, RoutedEventArgs e)
        {
            _ = ExtensionsRemoveAsync(sender, e);
        }

        private async System.Threading.Tasks.Task ExtensionsRemoveAsync(object sender, RoutedEventArgs e)
        {
            await FillViewAsync();
        }
    }
}

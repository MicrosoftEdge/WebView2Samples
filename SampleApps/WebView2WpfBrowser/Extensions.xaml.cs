using System;
using System.Collections.Generic;
using System.Text;
using System.Threading.Tasks;
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
            IReadOnlyList<CoreWebView2BrowserExtension> extensionsList = await m_coreWebView2.Profile.GetBrowserExtensionsAsync();

            m_listData.Clear();
            for (int i = 0; i < extensionsList.Count; ++i)
            {
                ListEntry entry = new ListEntry();
                entry.Name = extensionsList[i].Name;
                entry.Id = extensionsList[i].Id;
                entry.Enabled = extensionsList[i].IsEnabled;
                m_listData.Add(entry);
            }
            ExtensionsList.ItemsSource = m_listData;
            ExtensionsList.Items.Refresh();
        }

        private void ExtensionsToggleEnabled(object sender, RoutedEventArgs e)
        {
            _ = ExtensionsToggleEnabledAsync(sender, e);
        }

        private async System.Threading.Tasks.Task ExtensionsToggleEnabledAsync(object sender, RoutedEventArgs e)
        {
            ListEntry entry = (ListEntry)ExtensionsList.SelectedItem;
            IReadOnlyList<CoreWebView2BrowserExtension> extensionsList = await m_coreWebView2.Profile.GetBrowserExtensionsAsync();
            bool found = false;
            for (int i = 0; i < extensionsList.Count; ++i)
            {
                if (extensionsList[i].Id == entry.Id)
                {
                    try
                    {
                        await extensionsList[i].EnableAsync(extensionsList[i].IsEnabled ? false : true);
                    }
                    catch (Exception exception)
                    {
                        MessageBox.Show("Failed to toggle extension enabled: " + exception);
                    }
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                MessageBox.Show("Failed to find extension");
            }
            await FillViewAsync();
        }

        private void ExtensionsAdd(object sender, RoutedEventArgs e)
        {
            _ = ExtensionsAddAsync(sender, e);
        }

        private async System.Threading.Tasks.Task ExtensionsAddAsync(object sender, RoutedEventArgs e)
        {
            var dialog = new TextInputDialog(
                title: "Add extension",
                description: "Enter the absolute Windows file path to the unpackaged browser extension",
                defaultInput: "");
            if (dialog.ShowDialog() == true)
            {
                try
                {
                    CoreWebView2BrowserExtension extension = await m_coreWebView2.Profile.AddBrowserExtensionAsync(dialog.Input.Text);
                    MessageBox.Show("Added extension " + extension.Name + " (" + extension.Id + ")");
                    await FillViewAsync();
                }
                catch (Exception exception)
                {
                    MessageBox.Show("Failed to add extension: " + exception);
                }
            }
        }

        private void ExtensionsRemove(object sender, RoutedEventArgs e)
        {
            _ = ExtensionsRemoveAsync(sender, e);
        }

        private async System.Threading.Tasks.Task ExtensionsRemoveAsync(object sender, RoutedEventArgs e)
        {
            ListEntry entry = (ListEntry)ExtensionsList.SelectedItem;
            if (MessageBox.Show("Remove extension " + entry + "?", "Confirm removal", MessageBoxButton.OKCancel) == MessageBoxResult.OK)
            {
                IReadOnlyList<CoreWebView2BrowserExtension> extensionsList = await m_coreWebView2.Profile.GetBrowserExtensionsAsync();
                bool found = false;
                for (int i = 0; i < extensionsList.Count; ++i)
                {
                    if (extensionsList[i].Id == entry.Id)
                    {
                        try
                        {
                            await extensionsList[i].RemoveAsync();
                        }
                        catch (Exception exception)
                        {
                            MessageBox.Show("Failed to remove extension: " + exception);
                        }
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    MessageBox.Show("Failed to find extension");
                }
            }
            await FillViewAsync();
        }
    }
}

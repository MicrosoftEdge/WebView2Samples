using Microsoft.Web.WebView2.Core;
using System;
using System.Collections.Generic;
using System.Windows;

namespace WebView2WpfBrowser
{
    /// <summary>
    /// Interaction logic for ClientCertificateSelectionDialog.xaml
    /// </summary>
    public partial class ClientCertificateSelectionDialog : Window
    {
        public ClientCertificateSelectionDialog(
            string title = null,
            string host = null,
            int port = 0,
            IReadOnlyList<CoreWebView2ClientCertificate> client_cert_list = null)
        {
            InitializeComponent();
            if (title != null)
            {
                Title = title;
            }
            if (host != null && port > 0)
            {
                Description.Text = String.Format("Site {0}:{1} needs your credentials:", host, port);
            }
            if (client_cert_list != null)
            {
                CertificateDataBinding.SelectedItem = client_cert_list[0];
                CertificateDataBinding.ItemsSource = client_cert_list;
            }
        }
        void ok_Clicked(object sender, RoutedEventArgs args)
        {
            this.DialogResult = true;
        }
    }
}

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using Microsoft.Web.WebView2.Core;

namespace WebView2WindowsFormsBrowser
{
    public partial class ClientCertificateSelectionDialog : Form
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
                Text = title;
            }
            if (host != null && port > 0)
            {
                txtDescription.Text = String.Format("Site {0}:{1} needs your credentials:", host, port);
            }
            if (client_cert_list != null)
            {
                CertificateDataBinding.View = View.Details;
                CertificateDataBinding.FullRowSelect = true;
                CertificateDataBinding.Tag = client_cert_list;
                for(int i = 0; i < client_cert_list.Count; i++)
                {
                    ListViewItem item = new ListViewItem(client_cert_list[i].Subject);
                    item.Tag = client_cert_list[i];
                    item.SubItems.Add(client_cert_list[i].Issuer);
                    item.SubItems.Add(client_cert_list[i].ValidFrom.ToString());
                    item.SubItems.Add(client_cert_list[i].ValidTo.ToString());
                    item.SubItems.Add(client_cert_list[i].Kind.ToString());
                    CertificateDataBinding.Items.Add(item);
                }

                CertificateDataBinding.Columns.Add("Subject", -2, HorizontalAlignment.Left);
                CertificateDataBinding.Columns.Add("Issuer", -2, HorizontalAlignment.Left);
                CertificateDataBinding.Columns.Add("Valid From", -2, HorizontalAlignment.Left);
                CertificateDataBinding.Columns.Add("Valid To", -2, HorizontalAlignment.Left);
                CertificateDataBinding.Columns.Add("Kind", -2, HorizontalAlignment.Left);
            }
        }

        private void btnOk_Click(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.OK;
        }
        private void btnCancel_Click(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.Cancel;
        }
    }
}

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace WebView2WindowsFormsBrowser
{
    public partial class TextInputDialog : Form
    {
        public TextInputDialog(
            string title = null,
            string description = null,
            string defaultInput = null)
        {
            InitializeComponent();
            if (title != null)
            {
                Text = title;
            }
            if (description != null)
            {
                txtDescription.Text = description;
            }
            if (defaultInput != null)
            {
                txtInput.Text = defaultInput;
            }
            txtInput.Focus();
            txtInput.SelectAll();

        }

        public string inputBox()
        {
            return this.txtInput.Text;
        }

        void btnOk_Click(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.OK;
        }

        void btnCancel_Click(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.Cancel;
        }

    }
}

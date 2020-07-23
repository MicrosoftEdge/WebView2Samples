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

namespace WebView2WpfBrowser
{
    /// <summary>
    /// Interaction logic for TextInputDialog.xaml
    /// </summary>
    public partial class TextInputDialog : Window
    {
        public TextInputDialog(
            string title = null,
            string description = null,
            string defaultInput = null)
        {
            InitializeComponent();
            if (title != null) {
                Title = title;
            }
            if (description != null) {
                Description.Text = description;
            }
            if (defaultInput != null) {
                Input.Text = defaultInput;
            }
            Input.Focus();
            Input.SelectAll();
        }

        void ok_Clicked (object sender, RoutedEventArgs args) {
            this.DialogResult = true;
        }
    }
}

using System.Windows;

namespace WebView2WpfBrowser
{
    /// <summary>
    /// Collects the options used to create or join a shared cluster environment.
    ///
    /// Exposes only plain CLR types rather than
    /// CoreWebView2ClusterEnvironmentOptions, because those are experimental
    /// APIs absent from the stable package, so naming them here would fail the
    /// Stable configuration with CS0246. MainWindow converts inside its
    /// USE_WEBVIEW2_EXPERIMENTAL block.
    ///
    /// The defaults match the Win32 WebView2APISample so both samples pin an
    /// identical option set and can join each other's clusters.
    /// </summary>
    public partial class ClusterEnvironmentDialog : Window
    {
        public ClusterEnvironmentDialog(string defaultClusterName)
        {
            InitializeComponent();

            ClusterNameInput.Text = defaultClusterName;
            LanguageInput.Text = string.Empty;
            BrowserArgsInput.Text = string.Empty;

            SingleSignOnInput.IsChecked = false;
            TrackingPreventionInput.IsChecked = true;
            BrowserExtensionsInput.IsChecked = false;
            ProfileIsolationInput.IsChecked = true;

            StableInput.IsChecked = true;
            BetaInput.IsChecked = true;
            DevInput.IsChecked = true;
            CanaryInput.IsChecked = true;

            // LeastStable so the loader prefers a Canary/Dev channel install over
            // the stable Evergreen runtime, which predates the cluster feature.
            SearchKindInput.SelectedIndex = 1;

            ClusterNameInput.Focus();
            ClusterNameInput.SelectAll();
        }

        // Trimmed because the cluster name becomes a folder name: the loader
        // rejects control characters and a trailing space or dot.
        public string ClusterName => ClusterNameInput.Text.Trim();

        // Named ClusterLanguage rather than Language because FrameworkElement
        // already defines a Language property, and the sample builds with
        // TreatWarningsAsErrors so CS0108 would fail the build.
        public string ClusterLanguage => LanguageInput.Text.Trim();

        public string AdditionalBrowserArguments => BrowserArgsInput.Text.Trim();

        public bool AllowSingleSignOnUsingOSPrimaryAccount => SingleSignOnInput.IsChecked == true;

        public bool EnableTrackingPrevention => TrackingPreventionInput.IsChecked == true;

        public bool AreBrowserExtensionsEnabled => BrowserExtensionsInput.IsChecked == true;

        public bool PerHostProfileIsolation => ProfileIsolationInput.IsChecked == true;

        public bool StableChannel => StableInput.IsChecked == true;

        public bool BetaChannel => BetaInput.IsChecked == true;

        public bool DevChannel => DevInput.IsChecked == true;

        public bool CanaryChannel => CanaryInput.IsChecked == true;

        /// <summary>
        /// 0 = MostStable, 1 = LeastStable. Matches the values of
        /// CoreWebView2ChannelSearchKind without naming the experimental enum.
        /// </summary>
        public int ChannelSearchKindIndex => SearchKindInput.SelectedIndex;

        void ok_Clicked(object sender, RoutedEventArgs args)
        {
            if (string.IsNullOrWhiteSpace(ClusterNameInput.Text))
            {
                MessageBox.Show(this, "ClusterName is required.", "Shared Cluster Environment");
                // Put the caret back in the field that needs fixing, rather
                // than leaving a keyboard or screen reader user to find it.
                ClusterNameInput.Focus();
                ClusterNameInput.SelectAll();
                return;
            }
            DialogResult = true;
        }
    }
}

using Microsoft.UI.Xaml;
using System;
using System.Diagnostics;
using System.IO;
using Windows.Storage;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace WebView2_WinUI3_Sample
{
    /// <summary>
    /// Provides application-specific behavior to supplement the default Application class.
    /// </summary>
    public partial class App : Application
    {
        /// <summary>
        /// Initializes the singleton application object.  This is the first line of authored code
        /// executed, and as such is the logical equivalent of main() or WinMain().
        /// </summary>
        public App()
        {
            this.InitializeComponent();

            // If your shipping a fixed version WebView2 SDK with your application you will need
            // to use the following code (update the runtime version to what your shipping.

            //StorageFolder localFolder = Windows.ApplicationModel.Package.Current.InstalledLocation;
            //String fixedPath = Path.Combine(localFolder.Path, "FixedRuntime\\95.0.1020.53");
            //Debug.WriteLine($"Launch path [{localFolder.Path}]");
            //Debug.WriteLine($"FixedRuntime path [{fixedPath}]");
            //Environment.SetEnvironmentVariable("WEBVIEW2_BROWSER_EXECUTABLE_FOLDER", fixedPath);

        }

        /// <summary>
        /// Invoked when the application is launched normally by the end user.  Other entry points
        /// will be used such as when the application is launched to open a specific file.
        /// </summary>
        /// <param name="args">Details about the launch request and process.</param>
        protected override void OnLaunched(Microsoft.UI.Xaml.LaunchActivatedEventArgs args)
        {
            m_window = new MainWindow();
            m_window.Activate();
        }

        private Window m_window;
    }
}

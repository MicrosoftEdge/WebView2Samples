using System.Windows.Markup;

#if USE_WEBVIEW2_EXPERIMENTAL
[assembly: XmlnsDefinition("experimental-mode", "Namespace")]
#endif

#if USE_WEBVIEW2_INTERNAL
[assembly: XmlnsDefinition("internal-mode", "Namespace")]
#endif
